// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "ProjectEldoriaPlayerController.h"
#include "EldoriaPlayerController.generated.h"

class UEldoriaHUDWidget;
class UEldoriaInteractionComponent;

/**
 * Eldoria player controller: owns the gameplay HUD and connects it to the
 * possessed pawn's components (interaction prompt for now).
 * Input mapping contexts are still handled by the template parent class.
 */
UCLASS(abstract)
class AEldoriaPlayerController : public AProjectEldoriaPlayerController
{
	GENERATED_BODY()

public:

	AEldoriaPlayerController();

	UFUNCTION(BlueprintPure, Category="HUD")
	UEldoriaHUDWidget* GetHUDWidget() const { return HUDWidget; }

	/** Debug: collapses every collapsible structure along its forward vector. */
	UFUNCTION(Exec)
	void EldoriaCollapseAll();

	/** Debug: sets the possessed character's push strength (1 = base, FORCE will raise it). */
	UFUNCTION(Exec)
	void EldoriaSetStrength(float NewStrength);

	/** Debug: forces every deity into a state (0 Idle, 1 Roaming, 2 Investigating, 3 Alerted, 4 PreparingCharge, 5 Charging, 6 Recovering, 7 Stunned, 8 Liberated). */
	UFUNCTION(Exec)
	void EldoriaDeityState(int32 StateIndex);

	/** Debug: releases the next lock of every control device (then pulls the core). */
	UFUNCTION(Exec)
	void EldoriaAdvanceDevice();

	/** Debug: uses the character's primary ability (FORCE), as the ability key does. */
	UFUNCTION(Exec)
	void EldoriaUseAbility();

	/** Debug: hurts the character (quarter hearts). */
	UFUNCTION(Exec)
	void EldoriaDamage(int32 Quarters);

	/** Debug: holds the move stick (X right, Y forward) for Seconds, as if the player did. */
	UFUNCTION(Exec)
	void EldoriaHoldMove(float X, float Y, float Seconds);

	/** Debug: presses jump once. */
	UFUNCTION(Exec)
	void EldoriaJump();

	/** Debug: knocks every deity to its knees (heavy stun, as after ramming the tower down). */
	UFUNCTION(Exec)
	void EldoriaHeavyStun();

	/** Debug: teleports the character (metres) and faces it along Yaw. */
	UFUNCTION(Exec)
	void EldoriaTeleport(float X, float Y, float Z, float Yaw);

protected:

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** HUD widget class. Can be replaced with a Widget Blueprint deriving from UEldoriaHUDWidget. */
	UPROPERTY(EditAnywhere, Category="HUD")
	TSubclassOf<UEldoriaHUDWidget> HUDWidgetClass;

private:

	void BindToPawn(APawn* InPawn);
	void UnbindFromPawn();

	UFUNCTION()
	void HandleInteractionFocusChanged(UObject* FocusedTarget, const FText& Prompt);

	UFUNCTION()
	void HandleHealthChanged(int32 Quarters, int32 MaxQuarters);

	UFUNCTION()
	void HandleStaminaChanged(float Stamina, bool bExhausted);

	UPROPERTY()
	TObjectPtr<UEldoriaHUDWidget> HUDWidget;

	TWeakObjectPtr<UEldoriaInteractionComponent> BoundInteraction;
};
