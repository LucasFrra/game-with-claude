// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "ProjectEldoriaCharacter.h"
#include "EldoriaCharacter.generated.h"

class UEldoriaInteractionComponent;
class UEldoriaPushComponent;
class UEldoriaGrabComponent;
class UEldoriaAbilityComponent;
class UInputAction;

/**
 * Eldoria protagonist.
 *
 * Builds on the template third person character (movement, camera, jump) and adds:
 *  - sprinting,
 *  - environment interaction through UEldoriaInteractionComponent.
 *
 * Gameplay features live in components; this class only routes input and tunes movement.
 */
UCLASS(abstract)
class AEldoriaCharacter : public AProjectEldoriaCharacter
{
	GENERATED_BODY()

public:

	AEldoriaCharacter();

	UFUNCTION(BlueprintCallable, Category="Movement")
	void SetSprinting(bool bNewSprinting);

	UFUNCTION(BlueprintPure, Category="Movement")
	bool IsSprinting() const { return bIsSprinting; }

	/** Caps ground speed (pushing, carrying...). 0 removes the cap. */
	UFUNCTION(BlueprintCallable, Category="Movement")
	void SetMovementSpeedCap(float Cap);

	UEldoriaInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }
	UEldoriaPushComponent* GetPushComponent() const { return PushComponent; }

protected:

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void OnSprintStarted();
	void OnSprintStopped();
	void OnInteractStarted();
	void OnInteractStopped();

	void ApplyMovementSpeed();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEldoriaInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEldoriaPushComponent> PushComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEldoriaGrabComponent> GrabComponent;

	/** Throws the carried object */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ThrowAction;

	/** Uses the learnt divine ability */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> AbilityAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEldoriaAbilityComponent> AbilityComponent;

	void OnThrow();
	void OnAbility();

	/** Hold to sprint */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SprintAction;

	/** Press to interact (hold for held interactions such as pushing) */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float SprintSpeed = 800.f;

private:

	bool bIsSprinting = false;
	float SpeedCap = 0.f;
};
