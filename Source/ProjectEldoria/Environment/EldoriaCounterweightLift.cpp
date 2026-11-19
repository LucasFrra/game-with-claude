// Project Eldoria

#include "Environment/EldoriaCounterweightLift.h"
#include "UI/EldoriaMessageSubsystem.h"
#include "Physics/EldoriaPhysicsObject.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "EldoriaCounterweightLift"

AEldoriaCounterweightLift::AEldoriaCounterweightLift()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Platform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Platform"));
	Platform->SetupAttachment(Root);
	Platform->SetMobility(EComponentMobility::Movable);

	Counterweight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Counterweight"));
	Counterweight->SetupAttachment(Root);
	Counterweight->SetMobility(EComponentMobility::Movable);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Platform->SetStaticMesh(CubeMesh.Object);
		Platform->SetRelativeScale3D(FVector(3.f, 3.f, 0.3f));
		Counterweight->SetStaticMesh(CubeMesh.Object);
		Counterweight->SetRelativeScale3D(FVector(1.6f, 1.6f, 1.6f));
	}
}

void AEldoriaCounterweightLift::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	CurrentHeight = TargetHeight = 0.f;
	ApplyHeight();
}

void AEldoriaCounterweightLift::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(EvaluateTimer, this, &AEldoriaCounterweightLift::Evaluate, EvaluationInterval, true);
}

void AEldoriaCounterweightLift::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvaluateTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AEldoriaCounterweightLift::ApplyHeight()
{
	// Platform top sits at the actor origin when down
	const float PlatformHalfHeight = Platform->GetStaticMesh() ? Platform->GetStaticMesh()->GetBounds().BoxExtent.Z * Platform->GetRelativeScale3D().Z : 0.f;
	const float WeightHalfHeight = Counterweight->GetStaticMesh() ? Counterweight->GetStaticMesh()->GetBounds().BoxExtent.Z * Counterweight->GetRelativeScale3D().Z : 0.f;

	Platform->SetRelativeLocation(FVector(0.f, 0.f, CurrentHeight - PlatformHalfHeight));
	Counterweight->SetRelativeLocation(CounterweightOffset + FVector(0.f, 0.f, TravelHeight - CurrentHeight + WeightHalfHeight));
}

float AEldoriaCounterweightLift::MeasureLoad() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	const FBoxSphereBounds Bounds = Platform->Bounds;
	const FVector Center = Bounds.Origin + FVector(0.f, 0.f, Bounds.BoxExtent.Z + LoadDetectionHeight * 0.5f);
	const FVector Extent(Bounds.BoxExtent.X, Bounds.BoxExtent.Y, LoadDetectionHeight * 0.5f);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(EldoriaLiftLoad), false, this);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, Platform->GetComponentQuat(), ObjectParams, FCollisionShape::MakeBox(Extent), Params);

	float Load = 0.f;
	TSet<const AActor*> Counted;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Actor = Overlap.GetActor();
		const UPrimitiveComponent* Component = Overlap.GetComponent();
		if (!Actor || !Component || Counted.Contains(Actor))
		{
			continue;
		}

		if (const ACharacter* Character = Cast<ACharacter>(Actor))
		{
			Load += Character->GetCharacterMovement()->Mass;
			Counted.Add(Actor);
		}
		else if (const AEldoriaPhysicsObject* PhysicsObject = Cast<AEldoriaPhysicsObject>(Actor))
		{
			// Frozen (non simulating) objects still weigh their mass
			Load += PhysicsObject->GetMassKg();
			Counted.Add(Actor);
		}
		else if (Component->IsSimulatingPhysics())
		{
			Load += Component->GetMass();
			Counted.Add(Actor);
		}
	}
	return Load;
}

void AEldoriaCounterweightLift::Evaluate()
{
	const float Load = MeasureLoad();
	const bool bLightEnough = Load < CounterweightMassKg;

	float NewTarget = 0.f;
	if (bBrakeReleased)
	{
		NewTarget = bLightEnough ? TravelHeight : 0.f;

		if (!bLightEnough && !bWarnedOverload)
		{
			bWarnedOverload = true;
			UEldoriaMessageSubsystem::ShowMessage(this, LOCTEXT("Overload", "Les cordes se tendent... le contrepoids ne suffit pas à soulever cette charge."), 4.f);
		}
	}

	if (bLightEnough)
	{
		bWarnedOverload = false;
	}

	if (!FMath::IsNearlyEqual(NewTarget, TargetHeight))
	{
		TargetHeight = NewTarget;
		SetActorTickEnabled(true);
	}
}

void AEldoriaCounterweightLift::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CurrentHeight = FMath::FInterpConstantTo(CurrentHeight, TargetHeight, DeltaSeconds, Speed);
	ApplyHeight();

	if (FMath::IsNearlyEqual(CurrentHeight, TargetHeight))
	{
		SetActorTickEnabled(false);
	}
}

FText AEldoriaCounterweightLift::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return bBrakeReleased ? LOCTEXT("Lower", "Serrer le frein (descendre)") : LOCTEXT("Raise", "Libérer le contrepoids (monter)");
}

void AEldoriaCounterweightLift::BeginInteract_Implementation(AActor* Interactor)
{
	SetActivated_Implementation(!bBrakeReleased, Interactor);
}

void AEldoriaCounterweightLift::SetActivated_Implementation(bool bActivated, AActor* InstigatorActor)
{
	bBrakeReleased = bActivated;
	bWarnedOverload = false;
	Evaluate();
}

#undef LOCTEXT_NAMESPACE
