// Project Eldoria

#include "Components/EldoriaInteractionComponent.h"
#include "Interactions/EldoriaInteractable.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UEldoriaInteractionComponent::UEldoriaInteractionComponent()
{
	// Scanning is timer driven: no tick needed.
	PrimaryComponentTick.bCanEverTick = false;

	ScanObjectTypes = {
		UEngineTypes::ConvertToObjectType(ECC_WorldStatic),
		UEngineTypes::ConvertToObjectType(ECC_WorldDynamic),
		UEngineTypes::ConvertToObjectType(ECC_PhysicsBody),
		UEngineTypes::ConvertToObjectType(ECC_Pawn)
	};
}

void UEldoriaInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		// Small random first delay so several scanners never run on the same frame.
		World->GetTimerManager().SetTimer(ScanTimer, this, &UEldoriaInteractionComponent::Scan, ScanInterval, true, FMath::FRandRange(0.f, ScanInterval));
	}
}

void UEldoriaInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimer);
	}
	EndInteract();
	Super::EndPlay(EndPlayReason);
}

bool UEldoriaInteractionComponent::CanInteractWith(UObject* Target) const
{
	return IsValid(Target)
		&& Target->Implements<UEldoriaInteractable>()
		&& IEldoriaInteractable::Execute_CanInteract(Target, GetOwner());
}

FText UEldoriaInteractionComponent::GetPromptFor(UObject* Target) const
{
	return IsValid(Target) ? IEldoriaInteractable::Execute_GetInteractionPrompt(Target, GetOwner()) : FText::GetEmpty();
}

void UEldoriaInteractionComponent::Scan()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!bInteractionEnabled || !Owner || !World)
	{
		return;
	}

	// During a held interaction (pushing...) the focus stays locked on the held object
	if (HeldTarget.IsValid())
	{
		SetFocus(HeldTarget.Get());
		return;
	}

	const FVector Origin = Owner->GetActorLocation();

	// Eye point for line of sight, and camera direction for facing
	FVector EyeLocation = Origin;
	FVector ViewForward = Owner->GetActorForwardVector();
	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		FRotator EyeRotation;
		Pawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);
		if (const AController* Controller = Pawn->GetController())
		{
			ViewForward = Controller->GetControlRotation().Vector();
		}
	}
	const FVector ActorForward2D = Owner->GetActorForwardVector().GetSafeNormal2D();
	const FVector ViewForward2D = ViewForward.GetSafeNormal2D();
	const float MinFacingDot = FMath::Cos(FMath::DegreesToRadians(MaxFacingAngle));

	FCollisionObjectQueryParams ObjectParams;
	for (const TEnumAsByte<EObjectTypeQuery>& ObjectType : ScanObjectTypes)
	{
		ObjectParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjectType));
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EldoriaInteractionScan), false, Owner);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(InteractionRadius), QueryParams);

	UObject* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	FVector BestPoint = FVector::ZeroVector;
	TSet<UObject*> Evaluated;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		UPrimitiveComponent* Component = Overlap.GetComponent();
		AActor* Actor = Overlap.GetActor();
		if (!Component || !Actor)
		{
			continue;
		}

		// A component implementing the interface takes priority over its actor
		UObject* Candidate = Component->Implements<UEldoriaInteractable>() ? static_cast<UObject*>(Component)
			: (Actor->Implements<UEldoriaInteractable>() ? static_cast<UObject*>(Actor) : nullptr);

		if (!Candidate || Evaluated.Contains(Candidate))
		{
			continue;
		}
		Evaluated.Add(Candidate);

		if (!CanInteractWith(Candidate))
		{
			continue;
		}

		// Closest point on the collision is far better than the pivot for large objects
		FVector Point;
		if (Component->GetClosestPointOnCollision(Origin, Point) < 0.f)
		{
			Point = Component->Bounds.Origin;
		}

		const float Distance = FVector::Dist(Origin, Point);
		if (Distance > InteractionRadius)
		{
			continue;
		}

		float Facing = 1.f;
		if (Distance > TouchDistance)
		{
			const FVector ToTarget = (Point - Origin).GetSafeNormal2D();
			Facing = FMath::Max(FVector::DotProduct(ActorForward2D, ToTarget), FVector::DotProduct(ViewForward2D, ToTarget));
			if (Facing < MinFacingDot)
			{
				continue;
			}
		}

		// Line of sight: whatever we hit first must belong to the candidate's actor
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, EyeLocation, Point, LineOfSightChannel, QueryParams) && Hit.GetActor() != Actor)
		{
			continue;
		}

		const float Score = DistanceWeight * (Distance / InteractionRadius) + (1.f - DistanceWeight) * (1.f - Facing);
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
			BestPoint = Point;
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugSphere(World, Origin, InteractionRadius, 16, FColor::Cyan, false, ScanInterval);
		if (BestTarget)
		{
			DrawDebugLine(World, EyeLocation, BestPoint, FColor::Green, false, ScanInterval, 0, 1.5f);
		}
	}
#endif

	SetFocus(BestTarget);
}

void UEldoriaInteractionComponent::SetFocus(UObject* NewTarget)
{
	UObject* OldTarget = FocusedTarget.Get();

	if (OldTarget != NewTarget)
	{
		// Losing focus interrupts a held interaction
		if (HeldTarget.IsValid() && HeldTarget.Get() == OldTarget)
		{
			EndInteract();
		}

		if (IsValid(OldTarget))
		{
			IEldoriaInteractable::Execute_SetInteractionFocus(OldTarget, GetOwner(), false);
		}

		FocusedTarget = NewTarget;
		CurrentPrompt = GetPromptFor(NewTarget);

		if (IsValid(NewTarget))
		{
			IEldoriaInteractable::Execute_SetInteractionFocus(NewTarget, GetOwner(), true);
		}

		OnFocusChanged.Broadcast(NewTarget, CurrentPrompt);
		return;
	}

	// Same target: its prompt may have changed (lever pulled, object now too heavy...)
	if (IsValid(NewTarget))
	{
		const FText NewPrompt = GetPromptFor(NewTarget);
		if (!NewPrompt.EqualTo(CurrentPrompt))
		{
			CurrentPrompt = NewPrompt;
			OnFocusChanged.Broadcast(NewTarget, CurrentPrompt);
		}
	}
}

void UEldoriaInteractionComponent::BeginInteract()
{
	UObject* Target = FocusedTarget.Get();
	if (!bInteractionEnabled || !CanInteractWith(Target))
	{
		return;
	}

	EndInteract();
	HeldTarget = Target;
	IEldoriaInteractable::Execute_BeginInteract(Target, GetOwner());

	// The interaction may have changed the target's state and prompt
	RefreshFocus();
}

void UEldoriaInteractionComponent::EndInteract()
{
	UObject* Target = HeldTarget.Get();
	HeldTarget.Reset();
	if (IsValid(Target) && Target->Implements<UEldoriaInteractable>())
	{
		IEldoriaInteractable::Execute_EndInteract(Target, GetOwner());
	}
}

void UEldoriaInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	bInteractionEnabled = bEnabled;
	if (!bEnabled)
	{
		EndInteract();
		SetFocus(nullptr);
	}
}

void UEldoriaInteractionComponent::RefreshFocus()
{
	Scan();
}
