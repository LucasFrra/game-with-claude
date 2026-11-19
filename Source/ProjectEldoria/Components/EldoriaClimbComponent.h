// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EldoriaClimbComponent.generated.h"

class ACharacter;
class UEldoriaVitalsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEldoriaClimbStateChanged, bool, bClimbing);

/**
 * BOTW-like climbing: walk or jump into any steep static surface at least head high to grab it,
 * move across it, pull yourself up at the top, leap up (or kick away) with jump, let go with interact.
 * Costs stamina; when it runs out you fall.
 *
 * Not climbable: pawns (Tharum...), simulated physics objects, anything tagged "NoClimb".
 */
UCLASS(ClassGroup=(Eldoria), meta=(BlueprintSpawnableComponent))
class UEldoriaClimbComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UEldoriaClimbComponent();

	/** World-space direction the player wants to move in (set every frame by the character). */
	void SetMoveIntent(const FVector& WorldDirection) { MoveIntent = WorldDirection; }

	/** Raw stick input while climbing (X right, Y up). */
	void SetClimbInput(const FVector2D& Input) { ClimbInput = Input; }

	/** Jump pressed: grab a wall in reach while airborne, or leap / kick away while climbing. Returns true if handled. */
	bool HandleJump();

	UFUNCTION(BlueprintCallable, Category="Climb")
	void LetGo();

	UFUNCTION(BlueprintPure, Category="Climb")
	bool IsClimbing() const { return bClimbing; }

	UFUNCTION(BlueprintPure, Category="Climb")
	bool IsMantling() const { return bMantling; }

	/** Climbing speed relative to max (for animation playback). */
	UFUNCTION(BlueprintPure, Category="Climb")
	float GetClimbSpeedRatio() const { return SpeedRatio; }

	UPROPERTY(BlueprintAssignable, Category="Climb")
	FEldoriaClimbStateChanged OnClimbStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb", meta=(Units="CentimetersPerSecond"))
	float ClimbSpeed = 175.f;

	/** Surfaces flatter than this (normal Z) are walked, not climbed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb")
	float MaxClimbNormalZ = 0.5f;

	/** Overhangs steeper than this cannot be held. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb")
	float MinClimbNormalZ = -0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb|Stamina") float StaminaPerSecondMoving = 0.10f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb|Stamina") float StaminaPerSecondHolding = 0.025f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Climb|Stamina") float StaminaPerLeap = 0.16f;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	bool FindWall(const FVector& From, const FVector& Direction, float Distance, FHitResult& OutHit) const;
	bool IsClimbable(const FHitResult& Hit) const;
	bool TryStart(const FVector& Direction);
	void StartClimbing(const FHitResult& Hit);
	void StopClimbing();
	bool TryMantle();
	void UpdateClimbing(float DeltaTime);

	UPROPERTY() TObjectPtr<ACharacter> Character;
	UPROPERTY() TObjectPtr<UEldoriaVitalsComponent> Vitals;

	FVector MoveIntent = FVector::ZeroVector;
	FVector2D ClimbInput = FVector2D::ZeroVector;
	FVector WallNormal = FVector::ForwardVector;
	bool bClimbing = false;
	bool bMantling = false;
	float PushingIntoWall = 0.f;
	float LeapTime = 0.f;
	float RegrabBlock = 0.f;
	float SpeedRatio = 0.f;
	FVector MantleStart = FVector::ZeroVector;
	FVector MantleEnd = FVector::ZeroVector;
	float MantleAlpha = 0.f;
};
