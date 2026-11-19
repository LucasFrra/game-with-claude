// Project Eldoria

#include "Components/EldoriaGrabComponent.h"
#include "Components/EldoriaPushComponent.h"
#include "Physics/EldoriaPhysicsObject.h"
#include "Characters/EldoriaCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UEldoriaGrabComponent::UEldoriaGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	// Soft spring: heavy objects lag and swing, light ones follow closely
	bSoftLinearConstraint = true;
	LinearStiffness = 900.f;
	LinearDamping = 160.f;
	bSoftAngularConstraint = true;
	AngularStiffness = 1200.f;
	AngularDamping = 300.f;
	bInterpolateTarget = true;
	InterpolationSpeed = 20.f;
}

float UEldoriaGrabComponent::GetStrength() const
{
	// Strength is shared with the push component so a single ability changes both
	const UEldoriaPushComponent* Push = GetOwner() ? GetOwner()->FindComponentByClass<UEldoriaPushComponent>() : nullptr;
	return Push ? Push->GetStrength() : 1.f;
}

EEldoriaGrabMode UEldoriaGrabComponent::GetModeFor(const AEldoriaPhysicsObject* Object) const
{
	if (!Object || !Object->IsGrabbable())
	{
		return EEldoriaGrabMode::None;
	}

	const float Strength = GetStrength();
	const float Mass = Object->GetMassKg();
	if (Mass <= CarryMassPerStrength * Strength)
	{
		return EEldoriaGrabMode::Carry;
	}
	if (Mass <= DragMassPerStrength * Strength)
	{
		return EEldoriaGrabMode::Drag;
	}
	return EEldoriaGrabMode::None;
}

void UEldoriaGrabComponent::SetOwnerSpeedCap(float Cap) const
{
	if (AEldoriaCharacter* Character = Cast<AEldoriaCharacter>(GetOwner()))
	{
		Character->SetMovementSpeedCap(Cap);
	}
}

FVector UEldoriaGrabComponent::ComputeCarryTarget() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const AEldoriaPhysicsObject* Object = HeldObject.Get();
	if (!Character || !Object)
	{
		return FVector::ZeroVector;
	}

	const float ObjectRadius = Object->GetMesh()->Bounds.SphereRadius;
	const float Forward = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() + CarryDistance + ObjectRadius * 0.6f;
	const FVector Facing = Character->GetActorForwardVector().GetSafeNormal2D();
	return Character->GetActorLocation() + Facing * Forward + FVector(0.f, 0.f, CarryHeight);
}

bool UEldoriaGrabComponent::StartGrab(AEldoriaPhysicsObject* Object)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const EEldoriaGrabMode Mode = GetModeFor(Object);
	if (!Character || Mode == EEldoriaGrabMode::None)
	{
		return false;
	}

	Release();

	UStaticMeshComponent* Body = Object->GetMesh();
	Object->SetBeingHandled(true);

	SavedLinearDamping = Body->GetLinearDamping();
	SavedAngularDamping = Body->GetAngularDamping();

	if (Mode == EEldoriaGrabMode::Carry)
	{
		// The carried object must not push its carrier around
		Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Body->SetAngularDamping(4.f);
		GrabComponentAtLocationWithRotation(Body, NAME_None, Body->GetCenterOfMass(), Body->GetComponentRotation());

		// Heavier carried objects slow the character down
		const float Load = Object->GetMassKg() / FMath::Max(CarryMassPerStrength * GetStrength(), 1.f);
		SetOwnerSpeedCap(FMath::Lerp(500.f, 300.f, FMath::Clamp(Load, 0.f, 1.f)));
	}
	else
	{
		// Drag from the closest point of the object, at its current height
		FVector GrabPoint;
		if (Body->GetClosestPointOnCollision(Character->GetActorLocation(), GrabPoint) < 0.f)
		{
			GrabPoint = Body->GetCenterOfMass();
		}
		const FRotator Yaw(0.f, Character->GetActorRotation().Yaw, 0.f);
		DragLocalOffset = Yaw.UnrotateVector(GrabPoint - Character->GetActorLocation());
		Body->SetLinearDamping(0.5f);
		GrabComponentAtLocation(Body, NAME_None, GrabPoint);

		const float Load = Object->GetMassKg() / FMath::Max(DragMassPerStrength * GetStrength(), 1.f);
		SetOwnerSpeedCap(FMath::Lerp(MaxDragSpeed, MinDragSpeed, FMath::Clamp(Load, 0.f, 1.f)));
	}

	HeldObject = Object;
	CurrentMode = Mode;
	SetComponentTickEnabled(true);
	OnGrabChanged.Broadcast(CurrentMode, Object);
	return true;
}

void UEldoriaGrabComponent::Release()
{
	AEldoriaPhysicsObject* Object = HeldObject.Get();
	const EEldoriaGrabMode OldMode = CurrentMode;

	if (GetGrabbedComponent())
	{
		ReleaseComponent();
	}

	HeldObject.Reset();
	CurrentMode = EEldoriaGrabMode::None;
	SetComponentTickEnabled(false);
	SetOwnerSpeedCap(0.f);

	if (Object)
	{
		UStaticMeshComponent* Body = Object->GetMesh();
		Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Body->SetLinearDamping(SavedLinearDamping);
		Body->SetAngularDamping(SavedAngularDamping);
		Object->SetBeingHandled(false);
	}

	if (OldMode != EEldoriaGrabMode::None)
	{
		OnGrabChanged.Broadcast(EEldoriaGrabMode::None, Object);
	}
}

void UEldoriaGrabComponent::Throw()
{
	AEldoriaPhysicsObject* Object = HeldObject.Get();
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Object || !Character || CurrentMode != EEldoriaGrabMode::Carry)
	{
		return;
	}

	Release();

	// Lighter objects fly further; momentum budget scales with strength
	const float Ratio = FMath::Clamp(CarryMassPerStrength * GetStrength() / FMath::Max(Object->GetMassKg(), 1.f), 0.5f, 2.f);
	const FVector Direction = (Character->GetActorForwardVector().GetSafeNormal2D() + FVector(0.f, 0.f, 0.35f)).GetSafeNormal();
	Object->GetMesh()->AddImpulse(Direction * ThrowSpeed * Ratio, NAME_None, true);
}

void UEldoriaGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	AEldoriaPhysicsObject* Object = HeldObject.Get();
	if (!Character || !Object || !GetGrabbedComponent())
	{
		Release();
		return;
	}

	FVector Target;
	if (CurrentMode == EEldoriaGrabMode::Carry)
	{
		Target = ComputeCarryTarget();
		SetTargetLocationAndRotation(Target, FRotator(0.f, Character->GetActorRotation().Yaw, 0.f));
	}
	else
	{
		const FRotator Yaw(0.f, Character->GetActorRotation().Yaw, 0.f);
		// The grab point keeps its height relative to the character: we pull, we do not lift
		Target = Character->GetActorLocation() + Yaw.RotateVector(DragLocalOffset);
		SetTargetLocation(Target);

		// Dragging needs both feet on the ground
		if (!Character->GetCharacterMovement()->IsMovingOnGround())
		{
			Release();
			return;
		}
	}

	// The object got stuck or knocked away
	if (FVector::Dist(Object->GetMesh()->GetCenterOfMass(), Target) > BreakDistance + Object->GetMesh()->Bounds.SphereRadius)
	{
		Release();
	}
}

void UEldoriaGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Release();
	Super::EndPlay(EndPlayReason);
}
