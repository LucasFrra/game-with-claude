// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "EldoriaInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaInteractionFocusChanged, UObject*, FocusedTarget, const FText&, Prompt);

/**
 * Finds the best interactable around its owner and routes interact input to it.
 *
 * - Scans on a timer (not every frame) with a sphere overlap around the owner.
 * - Candidates are Actors or Components implementing IEldoriaInteractable.
 * - Candidates are scored by distance and by how much the owner/camera faces them,
 *   and must be in line of sight.
 * - Broadcasts OnFocusChanged whenever the focused object or its prompt changes,
 *   so any UI can listen without the component knowing about it.
 */
UCLASS(ClassGroup=(Eldoria), meta=(BlueprintSpawnableComponent))
class UEldoriaInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UEldoriaInteractionComponent();

	/** Interact input pressed. */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void BeginInteract();

	/** Interact input released. */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void EndInteract();

	/** Enable / disable scanning (cutscenes, menus...). Clears the current focus when disabled. */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetInteractionEnabled(bool bEnabled);

	/** Forces an immediate rescan (e.g. after an interactable changed state). */
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void RefreshFocus();

	UFUNCTION(BlueprintPure, Category="Interaction")
	UObject* GetFocusedTarget() const { return FocusedTarget.Get(); }

	UFUNCTION(BlueprintPure, Category="Interaction")
	bool IsInteracting() const { return HeldTarget.IsValid(); }

	/** Fired when the focused interactable or its prompt changes. Target is null when nothing is focused. */
	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FEldoriaInteractionFocusChanged OnFocusChanged;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Max distance (cm) from the owner to the closest point of an interactable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(ClampMin=50, Units="cm"))
	float InteractionRadius = 220.f;

	/** Seconds between two scans. Lower is more responsive but costs more queries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(ClampMin=0.02, Units="s"))
	float ScanInterval = 0.1f;

	/** Targets outside this half-angle (from the character facing or camera facing) are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(ClampMin=0, ClampMax=180, Units="deg"))
	float MaxFacingAngle = 75.f;

	/** Closer than this, facing is ignored (we are touching the object). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(ClampMin=0, Units="cm"))
	float TouchDistance = 60.f;

	/** Relative weight of distance vs facing when choosing between several candidates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(ClampMin=0, ClampMax=1))
	float DistanceWeight = 0.5f;

	/** Object types considered by the overlap scan. */
	UPROPERTY(EditAnywhere, Category="Interaction")
	TArray<TEnumAsByte<EObjectTypeQuery>> ScanObjectTypes;

	/** Channel used for the line of sight check. */
	UPROPERTY(EditAnywhere, Category="Interaction")
	TEnumAsByte<ECollisionChannel> LineOfSightChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category="Interaction|Debug")
	bool bDrawDebug = false;

private:

	void Scan();
	void SetFocus(UObject* NewTarget);
	FText GetPromptFor(UObject* Target) const;
	bool CanInteractWith(UObject* Target) const;

	FTimerHandle ScanTimer;

	TWeakObjectPtr<UObject> FocusedTarget;
	TWeakObjectPtr<UObject> HeldTarget;
	FText CurrentPrompt;
	bool bInteractionEnabled = true;
};
