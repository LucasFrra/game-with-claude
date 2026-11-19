// Project Eldoria

#include "Components/EldoriaPushComponent.h"
#include "Physics/EldoriaPhysicsObject.h"
#include "Characters/EldoriaCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UEldoriaPushComponent::UEldoriaPushComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

bool UEldoriaPushComponent::HasStrengthFor(const AEldoriaPhysicsObject* Object) const
{
	return Object && Object->IsPushable() && Strength >= Object->GetRequiredStrength();
}

bool UEldoriaPushComponent::StartPushing(AEldoriaPhysicsObject* Object)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !HasStrengthFor(Object) || !Character->GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}

	StopPushing();

	PushedObject = Object;
	PushDirection = ComputePushDirection(Object);
	Object->SetBeingHandled(true);

	// Boxes slide on their face rather than spinning under an off-centre push
	SavedAngularDamping = Object->GetMesh()->GetAngularDamping();
	if (Object->ShouldSnapPushToAxes())
	{
		Object->GetMesh()->SetAngularDamping(3.f);
	}

	// Slightly faster than the object so the character keeps contact
	SetOwnerSpeedCap(Object->GetPushSpeed() * 1.1f);
	SetComponentTickEnabled(true);

	OnPushStateChanged.Broadcast(true, Object);
	return true;
}

void UEldoriaPushComponent::StopPushing()
{
	AEldoriaPhysicsObject* Object = PushedObject.Get();
	PushedObject.Reset();
	SetComponentTickEnabled(false);
	SetOwnerSpeedCap(0.f);

	if (Object)
	{
		Object->GetMesh()->SetAngularDamping(SavedAngularDamping);
		Object->SetBeingHandled(false);
		OnPushStateChanged.Broadcast(false, Object);
	}
}

void UEldoriaPushComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopPushing();
	Super::EndPlay(EndPlayReason);
}

FVector UEldoriaPushComponent::ComputePushDirection(const AEldoriaPhysicsObject* Object) const
{
	const FVector From = GetOwner()->GetActorLocation();
	const FVector ToObject = (Object->GetMesh()->Bounds.Origin - From).GetSafeNormal2D();

	if (!Object->ShouldSnapPushToAxes())
	{
		return ToObject;
	}

	// Snap to the object's face we are pushing: the closest of its 4 horizontal axes
	const FVector Axes[2] = { Object->GetActorForwardVector().GetSafeNormal2D(), Object->GetActorRightVector().GetSafeNormal2D() };
	FVector Best = ToObject;
	float BestDot = -1.f;
	for (const FVector& Axis : Axes)
	{
		for (const float Sign : { 1.f, -1.f })
		{
			const float Dot = FVector::DotProduct(Axis * Sign, ToObject);
			if (Dot > BestDot)
			{
				BestDot = Dot;
				Best = Axis * Sign;
			}
		}
	}
	return Best;
}

void UEldoriaPushComponent::SetOwnerSpeedCap(float Cap)
{
	if (AEldoriaCharacter* Character = Cast<AEldoriaCharacter>(GetOwner()))
	{
		Character->SetMovementSpeedCap(Cap);
	}
}

void UEldoriaPushComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	AEldoriaPhysicsObject* Object = PushedObject.Get();
	if (!Character || !Object || !HasStrengthFor(Object) || !Character->GetCharacterMovement()->IsMovingOnGround())
	{
		StopPushing();
		return;
	}

	// Keep contact: distance from the capsule surface to the object's surface
	UStaticMeshComponent* Body = Object->GetMesh();
	const FVector CharacterLocation = Character->GetActorLocation();
	FVector Closest;
	if (Body->GetClosestPointOnCollision(CharacterLocation, Closest) < 0.f)
	{
		Closest = Body->Bounds.Origin;
	}
	const float Gap = FVector::Dist2D(CharacterLocation, Closest) - Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	if (Gap > MaxContactDistance)
	{
		StopPushing();
		return;
	}

	// Round objects can change direction as the character walks around them
	if (!Object->ShouldSnapPushToAxes())
	{
		PushDirection = ComputePushDirection(Object);
	}

	FVector Input = Character->GetLastMovementInputVector();
	Input.Z = 0.f;
	const float InputSize = FMath::Min(Input.Size(), 1.f);
	const float Alignment = InputSize > 0.1f ? FVector::DotProduct(Input.GetSafeNormal(), PushDirection) : 0.f;

	// Real force: the object accelerates according to its mass and friction, and keeps its momentum
	if (Alignment >= MinInputAlignment)
	{
		Object->ApplyPushForce(PushDirection, Strength * InputSize);
	}
}
