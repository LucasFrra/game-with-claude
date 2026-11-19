// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "EldoriaGrabComponent.generated.h"

class AEldoriaPhysicsObject;

UENUM(BlueprintType)
enum class EEldoriaGrabMode : uint8
{
	None,
	/** Light object held in front of the character, swings and collides. Can be thrown. */
	Carry,
	/** Medium object dragged along the ground while the interact input is held. */
	Drag
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaGrabChanged, EEldoriaGrabMode, Mode, AEldoriaPhysicsObject*, Object);

/**
 * Physical grabbing built on Unreal's UPhysicsHandleComponent: the object stays fully simulated
 * and is pulled by a soft spring, so mass, inertia and collisions all come from Chaos.
 *
 * What the character can do with an object depends on its mass relative to the character's strength:
 *   mass <= CarryMassPerStrength * Strength  -> carry
 *   mass <= DragMassPerStrength  * Strength  -> drag
 *   heavier                                   -> push (UEldoriaPushComponent) or nothing
 * Abilities only have to change the strength (shared with the push component).
 */
UCLASS(ClassGroup=(Eldoria), meta=(BlueprintSpawnableComponent))
class UEldoriaGrabComponent : public UPhysicsHandleComponent
{
	GENERATED_BODY()

public:

	UEldoriaGrabComponent();

	/** What this character would do with the object (None if too heavy to grab). */
	UFUNCTION(BlueprintPure, Category="Grab")
	EEldoriaGrabMode GetModeFor(const AEldoriaPhysicsObject* Object) const;

	UFUNCTION(BlueprintCallable, Category="Grab")
	bool StartGrab(AEldoriaPhysicsObject* Object);

	UFUNCTION(BlueprintCallable, Category="Grab")
	void Release();

	/** Throws the carried object forward. */
	UFUNCTION(BlueprintCallable, Category="Grab")
	void Throw();

	UFUNCTION(BlueprintPure, Category="Grab")
	EEldoriaGrabMode GetCurrentMode() const { return CurrentMode; }

	UFUNCTION(BlueprintPure, Category="Grab")
	AEldoriaPhysicsObject* GetHeldObject() const { return HeldObject.Get(); }

	UPROPERTY(BlueprintAssignable, Category="Grab")
	FEldoriaGrabChanged OnGrabChanged;

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab", meta=(ClampMin=0, Units="kg"))
	float CarryMassPerStrength = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab", meta=(ClampMin=0, Units="kg"))
	float DragMassPerStrength = 400.f;

	/** Carried objects are held this far in front of the capsule surface, at this height. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab|Carry", meta=(Units="cm"))
	float CarryDistance = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab|Carry", meta=(Units="cm"))
	float CarryHeight = 55.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab|Carry", meta=(Units="CentimetersPerSecond"))
	float ThrowSpeed = 900.f;

	/** Walk speed while dragging the heaviest draggable object (lighter objects are dragged faster). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab|Drag", meta=(Units="CentimetersPerSecond"))
	float MinDragSpeed = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab|Drag", meta=(Units="CentimetersPerSecond"))
	float MaxDragSpeed = 300.f;

	/** The grab breaks if the object lags further than this behind its target (stuck on something). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grab", meta=(Units="cm"))
	float BreakDistance = 220.f;

private:

	float GetStrength() const;
	void SetOwnerSpeedCap(float Cap) const;
	FVector ComputeCarryTarget() const;

	TWeakObjectPtr<AEldoriaPhysicsObject> HeldObject;
	EEldoriaGrabMode CurrentMode = EEldoriaGrabMode::None;

	/** Drag: grab point offset relative to the character, in the character's yaw space. */
	FVector DragLocalOffset = FVector::ZeroVector;
	float SavedLinearDamping = 0.f;
	float SavedAngularDamping = 0.f;
};
