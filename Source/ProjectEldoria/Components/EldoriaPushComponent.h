// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EldoriaPushComponent.generated.h"

class AEldoriaPhysicsObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaPushStateChanged, bool, bIsPushing, AEldoriaPhysicsObject*, Object);

/**
 * Lets a character physically push AEldoriaPhysicsObjects.
 *
 * While pushing (interact held), movement input pointing into the object drives it at its
 * push speed; the character is slowed to follow it. The object stays physically simulated:
 * it can fall off ledges, hit things, or be knocked away.
 *
 * Strength is the hook for progression: objects declare a RequiredStrength,
 * abilities (FORCE) raise the character's strength. Ticks only while pushing.
 */
UCLASS(ClassGroup=(Eldoria), meta=(BlueprintSpawnableComponent))
class UEldoriaPushComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UEldoriaPushComponent();

	UFUNCTION(BlueprintCallable, Category="Push")
	bool StartPushing(AEldoriaPhysicsObject* Object);

	UFUNCTION(BlueprintCallable, Category="Push")
	void StopPushing();

	UFUNCTION(BlueprintPure, Category="Push")
	bool HasStrengthFor(const AEldoriaPhysicsObject* Object) const;

	UFUNCTION(BlueprintPure, Category="Push")
	bool IsPushing() const { return PushedObject.IsValid(); }

	UFUNCTION(BlueprintPure, Category="Push")
	AEldoriaPhysicsObject* GetPushedObject() const { return PushedObject.Get(); }

	UFUNCTION(BlueprintPure, Category="Push")
	float GetStrength() const { return Strength; }

	/** Abilities use this to change what the character can move. */
	UFUNCTION(BlueprintCallable, Category="Push")
	void SetStrength(float NewStrength) { Strength = FMath::Max(0.f, NewStrength); }

	UPROPERTY(BlueprintAssignable, Category="Push")
	FEldoriaPushStateChanged OnPushStateChanged;

protected:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Base pushing strength. 1 = ordinary human. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Push", meta=(ClampMin=0))
	float Strength = 1.f;

	/** Pushing stops when the character is further than this from the object's surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Push", meta=(ClampMin=10, Units="cm"))
	float MaxContactDistance = 90.f;

	/** How much the movement input must point into the object to push (dot product). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Push", meta=(ClampMin=0, ClampMax=1))
	float MinInputAlignment = 0.5f;

private:

	FVector ComputePushDirection(const AEldoriaPhysicsObject* Object) const;
	void SetOwnerSpeedCap(float Cap);

	TWeakObjectPtr<AEldoriaPhysicsObject> PushedObject;
	FVector PushDirection = FVector::ForwardVector;
	float SavedAngularDamping = 0.f;
};
