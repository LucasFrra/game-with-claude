// Project Eldoria

#include "Environment/EldoriaLever.h"
#include "Interactions/EldoriaActivatable.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AEldoriaLever::AEldoriaLever()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(Root);

	HandlePivot = CreateDefaultSubobject<USceneComponent>(TEXT("HandlePivot"));
	HandlePivot->SetupAttachment(Root);
	HandlePivot->SetRelativeLocation(FVector(0.f, 0.f, 40.f));

	HandleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HandleMesh"));
	HandleMesh->SetupAttachment(HandlePivot);

	// Greybox defaults from engine basic shapes; replaceable in Blueprints
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CubeMesh.Succeeded())
	{
		BaseMesh->SetStaticMesh(CubeMesh.Object);
		BaseMesh->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
		BaseMesh->SetRelativeScale3D(FVector(0.7f, 0.5f, 0.4f));
	}
	if (CylinderMesh.Succeeded())
	{
		HandleMesh->SetStaticMesh(CylinderMesh.Object);
		HandleMesh->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
		HandleMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 1.1f));
	}
}

void AEldoriaLever::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	AnimAlpha = bIsOn ? 1.f : 0.f;
	ApplyHandleAngle(AnimAlpha);
}

bool AEldoriaLever::CanInteract_Implementation(AActor* Interactor) const
{
	return !IsActorTickEnabled() && (bCanToggleBack || !bIsOn);
}

FText AEldoriaLever::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return bIsOn ? PromptTurnOff : PromptTurnOn;
}

void AEldoriaLever::BeginInteract_Implementation(AActor* Interactor)
{
	SetLeverState(!bIsOn, Interactor);
}

void AEldoriaLever::SetLeverState(bool bNewOn, AActor* InstigatorActor)
{
	if (bNewOn == bIsOn)
	{
		return;
	}

	bIsOn = bNewOn;
	SetActorTickEnabled(true);

	for (AActor* Target : Targets)
	{
		if (IsValid(Target) && Target->Implements<UEldoriaActivatable>())
		{
			IEldoriaActivatable::Execute_SetActivated(Target, bIsOn, InstigatorActor);
		}
	}

	OnLeverToggled.Broadcast(bIsOn, InstigatorActor);
}

void AEldoriaLever::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Goal = bIsOn ? 1.f : 0.f;
	AnimAlpha = FMath::FInterpConstantTo(AnimAlpha, Goal, DeltaSeconds, 1.f / AnimationDuration);
	ApplyHandleAngle(AnimAlpha);

	if (FMath::IsNearlyEqual(AnimAlpha, Goal))
	{
		SetActorTickEnabled(false);
	}
}

void AEldoriaLever::ApplyHandleAngle(float Alpha)
{
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	HandlePivot->SetRelativeRotation(FRotator(FMath::Lerp(OffAngle, OnAngle, Eased), 0.f, 0.f));
}
