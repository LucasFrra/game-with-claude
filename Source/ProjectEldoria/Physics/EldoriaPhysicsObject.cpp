// Project Eldoria

#include "Physics/EldoriaPhysicsObject.h"
#include "Components/EldoriaPushComponent.h"
#include "Components/EldoriaGrabComponent.h"
#include "UI/EldoriaMessageSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "EldoriaPhysicsObject"

AEldoriaPhysicsObject::AEldoriaPhysicsObject()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetSimulatePhysics(true);
	// Tharum is kinematic: being overlapped by it must push objects, not fire them across the map
	Mesh->BodyInstance.SetMaxDepenetrationVelocity(500.f);
	Mesh->SetNotifyRigidBodyCollision(true);
	Mesh->SetLinearDamping(0.05f);
	Mesh->SetAngularDamping(0.2f);
	RootComponent = Mesh;
}

void AEldoriaPhysicsObject::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Mesh->SetMassOverrideInKg(NAME_None, MassKg, true);
}

void AEldoriaPhysicsObject::BeginPlay()
{
	Super::BeginPlay();

	Mesh->SetMassOverrideInKg(NAME_None, MassKg, true);
	Mesh->OnComponentHit.AddDynamic(this, &AEldoriaPhysicsObject::HandleHit);

	if (!bSimulateWhenIdle)
	{
		Mesh->SetSimulatePhysics(false);
	}
}

// --- Interaction -----------------------------------------------------------------------------

bool AEldoriaPhysicsObject::CanInteract_Implementation(AActor* Interactor) const
{
	return Interactor && (bGrabbable || bPushable)
		&& (Interactor->FindComponentByClass<UEldoriaGrabComponent>() || Interactor->FindComponentByClass<UEldoriaPushComponent>());
}

FText AEldoriaPhysicsObject::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	const UEldoriaGrabComponent* Grab = Interactor ? Interactor->FindComponentByClass<UEldoriaGrabComponent>() : nullptr;
	const UEldoriaPushComponent* Push = Interactor ? Interactor->FindComponentByClass<UEldoriaPushComponent>() : nullptr;

	if (Grab && Grab->GetHeldObject() == this)
	{
		return LOCTEXT("Drop", "Poser");
	}

	switch (Grab ? Grab->GetModeFor(this) : EEldoriaGrabMode::None)
	{
	case EEldoriaGrabMode::Carry:	return LOCTEXT("Carry", "Porter");
	case EEldoriaGrabMode::Drag:	return LOCTEXT("Drag", "Traîner (maintenir)");
	default:						break;
	}

	if (Push && bPushable)
	{
		return Push->HasStrengthFor(this) ? LOCTEXT("Push", "Pousser (maintenir)") : LOCTEXT("TooHeavyPrompt", "Pousser (trop lourd)");
	}
	return LOCTEXT("Immovable", "Trop lourd");
}

void AEldoriaPhysicsObject::BeginInteract_Implementation(AActor* Interactor)
{
	UEldoriaGrabComponent* Grab = Interactor ? Interactor->FindComponentByClass<UEldoriaGrabComponent>() : nullptr;
	UEldoriaPushComponent* Push = Interactor ? Interactor->FindComponentByClass<UEldoriaPushComponent>() : nullptr;

	// Carrying is a toggle: interacting again puts it down
	if (Grab && Grab->GetHeldObject() == this)
	{
		Grab->Release();
		return;
	}

	if (Grab && Grab->StartGrab(this))
	{
		return;
	}

	if (Push && bPushable)
	{
		if (!Push->HasStrengthFor(this))
		{
			UEldoriaMessageSubsystem::ShowMessage(this, LOCTEXT("TooHeavy", "Ça ne bouge pas d'un pouce. C'est bien trop lourd."), 3.f);
			return;
		}
		Push->StartPushing(this);
	}
}

void AEldoriaPhysicsObject::EndInteract_Implementation(AActor* Interactor)
{
	// Dragging and pushing last as long as the input is held; carrying does not
	if (UEldoriaGrabComponent* Grab = Interactor ? Interactor->FindComponentByClass<UEldoriaGrabComponent>() : nullptr)
	{
		if (Grab->GetHeldObject() == this && Grab->GetCurrentMode() == EEldoriaGrabMode::Drag)
		{
			Grab->Release();
		}
	}

	if (UEldoriaPushComponent* Push = Interactor ? Interactor->FindComponentByClass<UEldoriaPushComponent>() : nullptr)
	{
		if (Push->GetPushedObject() == this)
		{
			Push->StopPushing();
		}
	}
}

// --- Physics ---------------------------------------------------------------------------------

void AEldoriaPhysicsObject::Wake()
{
	if (!Mesh->IsSimulatingPhysics())
	{
		Mesh->SetSimulatePhysics(true);
		Mesh->SetMassOverrideInKg(NAME_None, MassKg, true);
	}
	Mesh->WakeRigidBody();

	// Frozen-when-idle objects go back to sleep once at rest
	if (!bSimulateWhenIdle && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(RestTimer))
	{
		RestChecks = 0;
		GetWorld()->GetTimerManager().SetTimer(RestTimer, this, &AEldoriaPhysicsObject::CheckForRest, 0.5f, true);
	}
}

void AEldoriaPhysicsObject::CheckForRest()
{
	if (bBeingHandled)
	{
		RestChecks = 0;
		return;
	}

	const bool bAtRest = Mesh->GetPhysicsLinearVelocity().SizeSquared() < 25.f && Mesh->GetPhysicsAngularVelocityInDegrees().SizeSquared() < 25.f;
	RestChecks = bAtRest ? RestChecks + 1 : 0;

	if (RestChecks >= 2)
	{
		GetWorld()->GetTimerManager().ClearTimer(RestTimer);
		Mesh->SetSimulatePhysics(false);
	}
}

void AEldoriaPhysicsObject::SetBeingHandled(bool bHandled)
{
	bBeingHandled = bHandled;
	if (bHandled)
	{
		Wake();
	}
}

float AEldoriaPhysicsObject::GetFrictionCoefficient() const
{
	const FBodyInstance* Body = Mesh->GetBodyInstance();
	const UPhysicalMaterial* Material = Body ? Body->GetSimplePhysicalMaterial() : nullptr;
	return Material ? Material->Friction : 0.7f;
}

void AEldoriaPhysicsObject::ApplyPushForce(const FVector& Direction, float StrengthFactor)
{
	Wake();

	// Already moving fast enough along the push: let inertia do the rest
	if (FVector::DotProduct(Mesh->GetPhysicsLinearVelocity(), Direction) >= PushSpeed)
	{
		return;
	}

	// Force = overcome sliding friction + accelerate. A stronger pusher accelerates it faster.
	const float Gravity = FMath::Abs(GetWorld()->GetGravityZ());
	const float RelativeStrength = FMath::Clamp(StrengthFactor / FMath::Max(RequiredStrength, 0.1f), 0.f, 3.f);
	const float Force = MassKg * (GetFrictionCoefficient() * Gravity + PushAcceleration * RelativeStrength) * FMath::Min(StrengthFactor, 1.f);

	// Push slightly below the centre of mass so tall objects do not tip towards the pusher
	const FVector Point = Mesh->GetCenterOfMass() - FVector(0.f, 0.f, Mesh->Bounds.BoxExtent.Z * 0.25f);
	Mesh->AddForceAtLocation(Direction * Force, Point);
}

void AEldoriaPhysicsObject::ReceiveImpact_Implementation(const FEldoriaImpact& Impact)
{
	Wake();

	// Convert part of the momentum into a velocity change, with a little lift so objects tumble
	const float DeltaV = FMath::Min(Impact.Strength * 100.f * ImpactTransfer / FMath::Max(MassKg, 1.f), MaxImpactVelocity);
	const FVector Direction = (Impact.Direction.GetSafeNormal2D() + FVector(0.f, 0.f, 0.35f)).GetSafeNormal();
	Mesh->AddImpulseAtLocation(Direction * DeltaV * MassKg, Impact.Location.IsZero() ? Mesh->GetComponentLocation() : Impact.Location);
}

void AEldoriaPhysicsObject::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Physics vs physics is already resolved by Chaos: reporting it too would double the effect
	if (!OtherActor || OtherActor == this || OtherActor->IsA<AEldoriaPhysicsObject>())
	{
		return;
	}

	// NormalImpulse is in kg*cm/s
	const float Strength = NormalImpulse.Size() / 100.f;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Strength < MinReportedImpact || Now - LastReportedImpactTime < 0.25)
	{
		return;
	}

	FEldoriaImpact Impact;
	Impact.Strength = Strength;
	Impact.Direction = Mesh->GetPhysicsLinearVelocity().GetSafeNormal();
	Impact.Location = Hit.ImpactPoint;
	Impact.Instigator = this;

	if (IEldoriaImpactReceiver::SendImpact(OtherActor, Impact))
	{
		LastReportedImpactTime = Now;
	}
}

#undef LOCTEXT_NAMESPACE
