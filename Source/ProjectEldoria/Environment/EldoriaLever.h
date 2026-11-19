// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactions/EldoriaInteractable.h"
#include "EldoriaLever.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaLeverToggled, bool, bIsOn, AActor*, InstigatorActor);

/**
 * Mechanical lever. Drives any number of IEldoriaActivatable targets (lifts, gates...).
 * Targets are set per instance in the level; Blueprints can also bind OnLeverToggled.
 */
UCLASS(Blueprintable)
class AEldoriaLever : public AActor, public IEldoriaInteractable
{
	GENERATED_BODY()

public:

	AEldoriaLever();

	// IEldoriaInteractable
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual void BeginInteract_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintCallable, Category="Lever")
	void SetLeverState(bool bNewOn, AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category="Lever")
	bool IsOn() const { return bIsOn; }

	UPROPERTY(BlueprintAssignable, Category="Lever")
	FEldoriaLeverToggled OnLeverToggled;

protected:

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	/** Rotates around its local Y axis (pitch). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> HandlePivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> HandleMesh;

	/** Actors implementing IEldoriaActivatable that this lever drives. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Lever")
	TArray<TObjectPtr<AActor>> Targets;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever")
	bool bIsOn = false;

	/** If false the lever can only be pulled once. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever")
	bool bCanToggleBack = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever")
	FText PromptTurnOn = NSLOCTEXT("EldoriaLever", "On", "Actionner le levier");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever")
	FText PromptTurnOff = NSLOCTEXT("EldoriaLever", "Off", "Remettre le levier");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever", meta=(Units="deg"))
	float OffAngle = -35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever", meta=(Units="deg"))
	float OnAngle = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever", meta=(ClampMin=0.05, Units="s"))
	float AnimationDuration = 0.35f;

private:

	void ApplyHandleAngle(float Alpha);

	float AnimAlpha = 0.f;	// 0 = off, 1 = on
};
