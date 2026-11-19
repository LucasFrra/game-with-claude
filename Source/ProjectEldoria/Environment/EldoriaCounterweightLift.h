// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactions/EldoriaInteractable.h"
#include "Interactions/EldoriaActivatable.h"
#include "EldoriaCounterweightLift.generated.h"

class UStaticMeshComponent;

/**
 * Pulley lift balanced by a counterweight.
 *
 * Releasing the brake (interacting with the lift, or an IEldoriaActivatable source such as a lever)
 * lets the counterweight pull the platform up - but only while the load on the platform is lighter
 * than the counterweight. Anything physical counts: the player, blocks, logs...
 * So the lift can carry the player, ferry a block up, or refuse an overloaded platform.
 * Engaging the brake again lowers the platform.
 */
UCLASS(Blueprintable)
class AEldoriaCounterweightLift : public AActor, public IEldoriaInteractable, public IEldoriaActivatable
{
	GENERATED_BODY()

public:

	AEldoriaCounterweightLift();

	// IEldoriaInteractable
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual void BeginInteract_Implementation(AActor* Interactor) override;

	// IEldoriaActivatable
	virtual void SetActivated_Implementation(bool bActivated, AActor* InstigatorActor) override;

	/** Total mass currently resting on the platform (kg). */
	UFUNCTION(BlueprintPure, Category="Lift")
	float MeasureLoad() const;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	void Evaluate();
	void ApplyHeight();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Platform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Counterweight;

	/** Vertical travel of the platform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift", meta=(ClampMin=50, Units="cm"))
	float TravelHeight = 1100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift", meta=(ClampMin=1, Units="kg"))
	float CounterweightMassKg = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift", meta=(ClampMin=10, Units="CentimetersPerSecond"))
	float Speed = 220.f;

	/** Where the counterweight hangs, relative to the actor (it moves opposite to the platform). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift")
	FVector CounterweightOffset = FVector(0.f, 450.f, 0.f);

	/** Height above the platform where loads are detected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift", meta=(ClampMin=20, Units="cm"))
	float LoadDetectionHeight = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift", meta=(ClampMin=0.05, Units="s"))
	float EvaluationInterval = 0.25f;

	/** True = brake released (platform pulled up if light enough). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lift")
	bool bBrakeReleased = false;

private:

	float CurrentHeight = 0.f;
	float TargetHeight = 0.f;
	bool bWarnedOverload = false;
	FTimerHandle EvaluateTimer;
};
