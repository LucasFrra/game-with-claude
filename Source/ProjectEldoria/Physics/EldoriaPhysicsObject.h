// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactions/EldoriaInteractable.h"
#include "Interactions/EldoriaImpactReceiver.h"
#include "EldoriaPhysicsObject.generated.h"

class UStaticMeshComponent;

/**
 * Reusable physical object: blocks, logs, crates, boulders...
 *
 * Everything is simulated by Chaos. What the player can do with it depends on its mass
 * compared with the player's strength (see UEldoriaGrabComponent / UEldoriaPushComponent):
 *   light  -> carry (and throw)        medium -> drag        heavy -> push        very heavy -> nothing
 * Pushing applies real forces (friction of the physical material + acceleration), so objects
 * have inertia, keep sliding a little, tip over edges and stop against obstacles.
 *
 * It also transmits strong impacts to IEldoriaImpactReceiver actors it hits, and reacts to impacts.
 * Objects can be frozen while idle (bSimulateWhenIdle = false) so that walking into them does nothing
 * and they cost nothing until something really moves them.
 */
UCLASS(Blueprintable)
class AEldoriaPhysicsObject : public AActor, public IEldoriaInteractable, public IEldoriaImpactReceiver
{
	GENERATED_BODY()

public:

	AEldoriaPhysicsObject();

	// IEldoriaInteractable
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual void BeginInteract_Implementation(AActor* Interactor) override;
	virtual void EndInteract_Implementation(AActor* Interactor) override;

	// IEldoriaImpactReceiver
	virtual void ReceiveImpact_Implementation(const FEldoriaImpact& Impact) override;

	/**
	 * Applies one frame of pushing force along Direction (horizontal, normalized).
	 * StrengthFactor = pusher strength * input amount. Does nothing above PushSpeed.
	 */
	void ApplyPushForce(const FVector& Direction, float StrengthFactor);

	/** Keeps the object awake (and simulating) while a character pushes/carries/drags it. */
	void SetBeingHandled(bool bHandled);

	UStaticMeshComponent* GetMesh() const { return Mesh; }
	float GetRequiredStrength() const { return RequiredStrength; }
	float GetPushSpeed() const { return PushSpeed; }
	bool ShouldSnapPushToAxes() const { return bSnapPushToAxes; }
	bool IsPushable() const { return bPushable; }
	bool IsGrabbable() const { return bGrabbable; }

	UFUNCTION(BlueprintPure, Category="Physics")
	float GetMassKg() const { return MassKg; }

protected:

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void Wake();
	void CheckForRest();
	float GetFrictionCoefficient() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Mass used by the simulation, by weight-sensing mechanisms, and to decide carry/drag/push. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics", meta=(ClampMin=1, Units="kg"))
	float MassKg = 200.f;

	/** If false, physics is off while the object is at rest: only a real push, grab or strong impact moves it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics")
	bool bSimulateWhenIdle = true;

	/** Can be carried / dragged when light enough for the character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Handling")
	bool bGrabbable = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Handling")
	bool bPushable = true;

	/** Strength needed to push it. Player base strength is 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Handling", meta=(ClampMin=0, EditCondition="bPushable"))
	float RequiredStrength = 1.f;

	/** Top speed reached when pushed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Handling", meta=(ClampMin=10, Units="CentimetersPerSecond", EditCondition="bPushable"))
	float PushSpeed = 120.f;

	/** Acceleration given by a push of exactly RequiredStrength (stronger pushers accelerate it faster). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Handling", meta=(ClampMin=1, EditCondition="bPushable"))
	float PushAcceleration = 220.f;

	/** Box-like objects are pushed along their faces; round objects (boulders) in any direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Handling", meta=(EditCondition="bPushable"))
	bool bSnapPushToAxes = true;

	/** Impacts we deal below this momentum (kg*m/s) are not reported to receivers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impact", meta=(ClampMin=0))
	float MinReportedImpact = 1500.f;

	/** Fraction of a received impact converted into velocity (0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impact", meta=(ClampMin=0, ClampMax=1))
	float ImpactTransfer = 0.2f;

	/** Max velocity change from a single received impact. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics|Impact", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float MaxImpactVelocity = 1400.f;

private:

	bool bBeingHandled = false;
	int32 RestChecks = 0;
	double LastReportedImpactTime = -100.0;
	FTimerHandle RestTimer;
};
