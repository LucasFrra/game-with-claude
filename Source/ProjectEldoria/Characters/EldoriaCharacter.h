// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "ProjectEldoriaCharacter.h"
#include "EldoriaCharacter.generated.h"

class UEldoriaInteractionComponent;
class UEldoriaPushComponent;
class UEldoriaGrabComponent;
class UEldoriaAbilityComponent;
class UEldoriaVitalsComponent;
class UEldoriaClimbComponent;
class UInputAction;
class UAnimSequenceBase;
class UMaterialParameterCollection;

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
	UEldoriaVitalsComponent* GetVitals() const { return VitalsComponent; }
	UEldoriaClimbComponent* GetClimb() const { return ClimbComponent; }

	/** Where the player comes back after dying or falling out of the world. */
	UFUNCTION(BlueprintCallable, Category="Respawn")
	void SetRespawnPoint(const FTransform& Point) { RespawnPoint = Point; }

	UFUNCTION(BlueprintCallable, Category="Respawn")
	void Respawn();

	/** Turns the camera towards Target for Seconds (boss intro, liberation...). */
	UFUNCTION(BlueprintCallable, Category="Camera")
	void FocusOn(AActor* Target, float Seconds);

	/** Debug / tests: feeds the move input (X right, Y forward) every frame for Seconds. */
	void HoldTestInput(const FVector2D& Input, float Seconds) { TestInput = Input; TestInputTime = Seconds; }

	virtual void FellOutOfWorld(const UDamageType& DamageType) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void Tick(float DeltaSeconds) override;
	UEldoriaPushComponent* GetPushComponent() const { return PushComponent; }

protected:

	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void DoMove(float Right, float Forward) override;
	virtual void DoJumpStart() override;
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;

	UFUNCTION()
	void HandleDied();

	UFUNCTION()
	void HandleStaminaChanged(float Stamina, bool bExhausted);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEldoriaVitalsComponent> VitalsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UEldoriaClimbComponent> ClimbComponent;

	/** Stamina per second while sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
	float SprintStaminaPerSecond = 0.12f;

	/** Walk speed while out of breath. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(Units="CentimetersPerSecond"))
	float ExhaustedSpeed = 300.f;

	/** Landing faster than this hurts (cm/s, ~15 m fall). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(Units="CentimetersPerSecond"))
	float SafeFallSpeed = 1700.f;

	// --- State animations (played over the locomotion Anim Blueprint while the state lasts)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TObjectPtr<UAnimSequenceBase> ClimbAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TObjectPtr<UAnimSequenceBase> ClimbHangAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TObjectPtr<UAnimSequenceBase> PushAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TObjectPtr<UAnimSequenceBase> CarryWalkAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TObjectPtr<UAnimSequenceBase> CarryIdleAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TObjectPtr<UAnimSequenceBase> HitReactAnim;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation") TObjectPtr<UAnimSequenceBase> ThrowAnim;

	/** Hold to lock the camera onto the nearest deity (Z-targeting). */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LockOnAction;

	/** Shared material parameters (player position for grass that bends away). */
	UPROPERTY(EditAnywhere, Category="Rendering")
	TObjectPtr<UMaterialParameterCollection> WorldParameters;

	/** Grace period to jump after walking off a ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(Units="s"))
	float CoyoteTime = 0.15f;

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
	bool bWasExhausted = false;
	float SpeedCap = 0.f;
	float LowestFallVelocityZ = 0.f;
	float LeftGroundTime = -1.f;
	bool bJumpedSinceGround = false;
	FTransform RespawnPoint;
	void UpdateStateAnimation(float DeltaSeconds);

	/** Adventurer outfit (hood, scarf, backpack, satchel), rigid pieces riding on the mannequin bones. */
	UPROPERTY(VisibleAnywhere, Category="Outfit") TArray<TObjectPtr<UStaticMeshComponent>> OutfitPieces;
	TArray<FName> OutfitBones;
	UStaticMeshComponent* AddOutfitPiece(FName Name, UStaticMesh* PieceMesh, FName Bone);
	/** Aligns each piece with the character (pieces are modelled in actor space around their bone). */
	void FitOutfit();
	void PlayOneShot(UAnimSequenceBase* Anim, float Duration);

	UFUNCTION()
	void HandleDamaged(int32 Quarters);

	UPROPERTY(Transient) TObjectPtr<UAnimSequenceBase> CurrentStateAnim;
	void UpdateCameraFeel(float DeltaSeconds);
	void OnLockOnStarted();
	void OnLockOnStopped();
	bool bLockOnHeld = false;
	TWeakObjectPtr<AActor> FocusTarget;
	float FocusTime = 0.f;
	float EffortTimer = 0.f;
	float HurtFlash = 0.f;
	float BaseArmLength = 450.f;
	float BaseFOV = 90.f;
	float OneShotTime = 0.f;
	FVector2D TestInput = FVector2D::ZeroVector;
	float TestInputTime = 0.f;
	FTimerHandle RespawnTimer;
};
