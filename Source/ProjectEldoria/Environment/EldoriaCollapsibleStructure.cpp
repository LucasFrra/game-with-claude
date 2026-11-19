// Project Eldoria

#include "Environment/EldoriaCollapsibleStructure.h"
#include "ProjectEldoria.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	const FName LooseTag(TEXT("Loose"));
}

AEldoriaCollapsibleStructure::AEldoriaCollapsibleStructure()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	ToppleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ToppleRoot"));
	ToppleRoot->SetupAttachment(Root);
	ToppleRoot->SetMobility(EComponentMobility::Movable);
}

void AEldoriaCollapsibleStructure::ReceiveImpact_Implementation(const FEldoriaImpact& Impact)
{
	if (!bCollapsed && Impact.Strength >= ImpactThreshold)
	{
		Collapse(Impact.Direction, Impact.Instigator);
	}
}

void AEldoriaCollapsibleStructure::DebugCollapse()
{
	Collapse(GetActorForwardVector(), nullptr);
}

void AEldoriaCollapsibleStructure::ReleaseAsDebris(UPrimitiveComponent* Piece, const FVector& Direction)
{
	Piece->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Piece->SetMobility(EComponentMobility::Movable);
	Piece->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Piece->SetSimulatePhysics(true);
	Piece->SetMassOverrideInKg(NAME_None, DebrisMassKg, true);
	// The deity is kinematic and moves by teleporting steps: never let it fling debris across the map
	Piece->BodyInstance.SetMaxDepenetrationVelocity(400.f);

	const FVector Spread = FMath::VRandCone(Direction, FMath::DegreesToRadians(35.f));
	Piece->AddImpulse((Spread + FVector(0.f, 0.f, 0.4f)).GetSafeNormal() * DebrisVelocity, NAME_None, true);
}

void AEldoriaCollapsibleStructure::Collapse(FVector Direction, AActor* InstigatorActor)
{
	if (bCollapsed)
	{
		return;
	}
	bCollapsed = true;
	CollapseInstigator = InstigatorActor;

	FallDirection = bUseFixedFallDirection ? GetActorTransform().TransformVectorNoScale(FixedFallDirection).GetSafeNormal2D() : Direction.GetSafeNormal2D();
	if (bFallTowardInstigator && InstigatorActor && InstigatorActor != this)
	{
		FallDirection = (InstigatorActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	}
	UE_LOG(LogProjectEldoria, Log, TEXT("[Collapse] %s falls toward %s (instigator %s)"), *GetName(), *FallDirection.ToCompactString(), *GetNameSafe(InstigatorActor));
	if (FallDirection.IsNearlyZero())
	{
		FallDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	TArray<UStaticMeshComponent*> Pieces;
	GetComponents<UStaticMeshComponent>(Pieces);

	if (CollapseMode == EEldoriaCollapseMode::Shatter)
	{
		for (UStaticMeshComponent* Piece : Pieces)
		{
			ReleaseAsDebris(Piece, FallDirection);
		}
		OnCollapsed.Broadcast(this);
		return;
	}

	// Topple: loose pieces become debris, the solid ones are welded into a single rigid body
	TArray<UStaticMeshComponent*> Solid;
	for (UStaticMeshComponent* Piece : Pieces)
	{
		if (Piece->ComponentHasTag(LooseTag))
		{
			ReleaseAsDebris(Piece, FallDirection);
		}
		else
		{
			Solid.Add(Piece);
		}
	}

	if (Solid.Num() == 0)
	{
		OnCollapsed.Broadcast(this);
		return;
	}

	// The biggest piece carries the others
	Solid.Sort([](const UStaticMeshComponent& A, const UStaticMeshComponent& B)
	{
		return A.Bounds.BoxExtent.SizeSquared() > B.Bounds.BoxExtent.SizeSquared();
	});
	UStaticMeshComponent* Main = Solid[0];
	MainBody = Main;

	FBox Box(ForceInit);
	for (UStaticMeshComponent* Piece : Solid)
	{
		Box += Piece->Bounds.GetBox();
		Piece->SetMobility(EComponentMobility::Movable);
		Piece->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	}

	Main->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	for (int32 Index = 1; Index < Solid.Num(); ++Index)
	{
		Solid[Index]->AttachToComponent(Main, FAttachmentTransformRules(EAttachmentRule::KeepWorld, /*bWeldSimulatedBodies*/ true));
	}

	Main->SetSimulatePhysics(true);
	Main->SetMassOverrideInKg(NAME_None, StructureMassKg, true);
	Main->BodyInstance.SetMaxDepenetrationVelocity(300.f);
	Main->SetNotifyRigidBodyCollision(true);
	Main->OnComponentHit.AddDynamic(this, &AEldoriaCollapsibleStructure::HandleBodyHit);
	Main->WakeRigidBody();

	// Kick the top: friction holds the base, so the structure pivots and falls under its own weight
	const FVector Top(Box.GetCenter().X, Box.GetCenter().Y, Box.Max.Z);
	Main->AddImpulseAtLocation(FallDirection * TopKickSpeed * StructureMassKg, Top);

	GetWorld()->GetTimerManager().SetTimer(SettleTimer, this, &AEldoriaCollapsibleStructure::CheckSettled, 0.5f, true, 1.f);
	OnCollapsed.Broadcast(this);
}

void AEldoriaCollapsibleStructure::CheckSettled()
{
	if (!MainBody || bSettled)
	{
		return;
	}

	const bool bAtRest = MainBody->GetPhysicsLinearVelocity().SizeSquared() < 100.f && MainBody->GetPhysicsAngularVelocityInDegrees().SizeSquared() < 25.f;
	RestChecks = bAtRest ? RestChecks + 1 : 0;

	if (RestChecks >= 3)
	{
		bSettled = true;
		GetWorld()->GetTimerManager().ClearTimer(SettleTimer);
		if (bFreezeWhenSettled)
		{
			MainBody->SetSimulatePhysics(false);
		}
		OnSettled.Broadcast(this);
	}
}

void AEldoriaCollapsibleStructure::HandleBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	const float Strength = NormalImpulse.Size() / 100.f;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Strength < MinReportedImpact || Now - LastReportedImpactTime < 0.3)
	{
		return;
	}

	FEldoriaImpact Impact;
	Impact.Strength = Strength;
	Impact.Direction = FallDirection;
	Impact.Location = Hit.ImpactPoint;
	Impact.Instigator = this;

	if (IEldoriaImpactReceiver::SendImpact(OtherActor, Impact))
	{
		LastReportedImpactTime = Now;
	}
}
