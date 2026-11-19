// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactions/EldoriaImpactReceiver.h"
#include "EldoriaDeityBase.generated.h"

class ACharacter;

UENUM(BlueprintType)
enum class EEldoriaDeityState : uint8
{
	Idle,
	Roaming,
	Investigating,
	Alerted,
	PreparingCharge,
	Charging,
	Recovering,
	Stunned,
	Liberated
};

/** A point of the deity's daily route. Editable in the viewport. */
USTRUCT(BlueprintType)
struct FEldoriaDeityWaypoint
{
	GENERATED_BODY()

	/** World location (only X/Y matter, the deity follows the ground). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint")
	FVector Location = FVector::ZeroVector;

	/** Time spent here before moving on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint", meta=(ClampMin=0, Units="s"))
	float WaitTime = 4.f;

	/** Lower the head while waiting (drinking, grazing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint")
	bool bLowerHead = false;

	/** Turn to face this yaw while waiting here (e.g. towards the water). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint")
	bool bUseFacing = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Waypoint", meta=(EditCondition="bUseFacing", Units="deg"))
	float FacingYaw = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaDeityStateChanged, EEldoriaDeityState, OldState, EEldoriaDeityState, NewState);

/**
 * Base class for the colossal deities.
 *
 * Lightweight state machine (no behaviour tree), ground-following movement driven by traces
 * (no navmesh for a creature this size), perception of the player, and a readable charge attack:
 * a long telegraph, a straight committed run, and consequences:
 *   - everything in front receives an impact (trees topple, ruins collapse, objects fly),
 *   - the player is knocked away,
 *   - a wall stops the charge and stuns the deity; ramming a structure stuns it too.
 * The deity itself is only stunned by strong impacts (a falling tower...), never by weapons.
 *
 * The body is built by subclasses (see AEldoriaTharum), which also drive the procedural animation
 * from the state, speed and the values exposed here.
 */
UCLASS(Abstract, Blueprintable)
class AEldoriaDeityBase : public AActor, public IEldoriaImpactReceiver
{
	GENERATED_BODY()

public:

	AEldoriaDeityBase();

	// IEldoriaImpactReceiver
	virtual void ReceiveImpact_Implementation(const FEldoriaImpact& Impact) override;

	UFUNCTION(BlueprintCallable, Category="Deity")
	void SetState(EEldoriaDeityState NewState);

	UFUNCTION(BlueprintPure, Category="Deity")
	EEldoriaDeityState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category="Deity")
	float GetStateTime() const { return StateTime; }

	UFUNCTION(BlueprintPure, Category="Deity")
	float GetCurrentSpeed() const { return CurrentSpeed; }

	/** Frees the deity from its control: it stops attacking for good. */
	UFUNCTION(BlueprintCallable, Category="Deity")
	void Liberate();

	/** Dazes the deity for Duration seconds (keeps the longest if already stunned). */
	UFUNCTION(BlueprintCallable, Category="Deity")
	void Stun(float Duration);

	/** True while the given actor stands on this deity's body. */
	UFUNCTION(BlueprintPure, Category="Deity")
	bool IsCarrying(const AActor* Actor) const;

	UPROPERTY(BlueprintAssignable, Category="Deity")
	FEldoriaDeityStateChanged OnStateChanged;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Subclasses animate their body here. Called every tick after movement. */
	virtual void UpdateBodyAnimation(float DeltaSeconds) {}

	/** Half size (cm) of the box swept in front of the head while charging. */
	virtual FVector GetChargeSweepExtent() const { return FVector(250.f, 450.f, 400.f); }

	/** Local offset of the front of the deity (where the charge hits). */
	virtual FVector GetChargeFrontOffset() const { return FVector(1300.f, 0.f, 600.f); }

	// --- Movement ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Movement", meta=(Units="CentimetersPerSecond"))
	float WalkSpeed = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Movement", meta=(Units="deg"))
	float TurnRate = 28.f;

	/** Height of the actor origin above the ground. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Movement", meta=(Units="cm"))
	float GroundOffset = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Deity|Movement", meta=(MakeEditWidget=true))
	TArray<FEldoriaDeityWaypoint> Waypoints;

	// --- Perception ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Perception", meta=(Units="cm"))
	float SightRadius = 5500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Perception", meta=(Units="deg"))
	float SightHalfAngle = 70.f;

	/** The player is always noticed closer than this (heard / felt). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Perception", meta=(Units="cm"))
	float HearingRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Perception", meta=(Units="s"))
	float LoseInterestTime = 6.f;

	/** The deity never leaves its domain: a player beyond this radius around TerritoryCenter is ignored,
	 *  and a charge ends at the edge. 0 disables the territory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Perception", meta=(Units="cm", ClampMin=0))
	float TerritoryRadius = 4000.f;

	/** Static obstacles lower than this (above the deity's feet) do not stop a charge: it walks over them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="cm", ClampMin=0))
	float MinWallHeight = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Perception")
	FVector TerritoryCenter = FVector::ZeroVector;

	UFUNCTION(BlueprintPure, Category="Deity")
	bool IsInTerritory(const FVector& Location) const
	{
		return TerritoryRadius <= 0.f || FVector::Dist2D(Location, TerritoryCenter) <= TerritoryRadius;
	}

	// --- Charge ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="cm"))
	float ChargeMinDistance = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="cm"))
	float ChargeMaxDistance = 5000.f;

	/** Telegraph length. Long on purpose: the player must be able to read and avoid it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="s"))
	float PrepareChargeTime = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="CentimetersPerSecond"))
	float ChargeSpeed = 1500.f;

	/** The run continues this far past the player's position. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="cm"))
	float ChargeOvershoot = 1200.f;

	/** Momentum (kg*m/s) sent to whatever the charge hits. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge")
	float ChargeImpactStrength = 60000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="CentimetersPerSecond"))
	float PlayerKnockbackSpeed = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="s"))
	float RecoverTime = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity|Charge", meta=(Units="s"))
	float StunDuration = 8.f;

	/** Stun duration when something massive lands on the deity (a toppling tower). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity", meta=(Units="s"))
	float HeavyStunDuration = 16.f;

	/** Impacts received above this momentum stun the deity (a collapsing tower...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Deity")
	float StunImpactThreshold = 25000.f;

	UPROPERTY(EditAnywhere, Category="Deity|Debug")
	bool bDrawDebug = false;

	// --- Runtime state exposed to subclasses for animation ---
	EEldoriaDeityState State = EEldoriaDeityState::Idle;
	float StateTime = 0.f;
	float CurrentSpeed = 0.f;
	bool bHeadLowered = false;
	float CurrentStunDuration = 8.f;

	/** Set by a heavy stun (rammed a massive structure down, struck by a collapse): the deity collapses to its knees. */
	bool bHeavyStun = false;

public:
	UFUNCTION(BlueprintPure, Category="Deity")
	bool IsHeavilyStunned() const { return State == EEldoriaDeityState::Stunned && bHeavyStun; }

protected:

private:

	void UpdatePerception(float DeltaSeconds);
	void UpdateStateMachine(float DeltaSeconds);
	void MoveTowards(const FVector& Target, float Speed, float DeltaSeconds, bool bAllowTurnOnly = false);
	void TurnTowards(const FVector& Target, float DeltaSeconds, float RateMultiplier = 1.f);
	void FollowGround();
	void UpdateCharge(float DeltaSeconds);
	bool IsFacing(const FVector& Target, float ToleranceDeg) const;

	/** True if a real wall (static, taller than MinWallHeight) stands between the deity and Target along its charge lane. */
	bool IsChargeLaneBlocked(const FVector& Target) const;

	/** Where the deity walks to get a clear charge lane after finding it blocked. */
	FVector FlankTarget = FVector::ZeroVector;
	float FlankTime = 0.f;
	float FlankSide = 1.f;

	TWeakObjectPtr<ACharacter> Player;
	bool bPlayerVisible = false;
	bool bWasVisible = false;
	float TimeSinceSeen = 1000.f;
	float PerceptionTimer = 0.f;
	FVector LastKnownPlayerLocation = FVector::ZeroVector;

	int32 WaypointIndex = 0;
	FVector ChargeDirection = FVector::ForwardVector;
	FVector ChargeStart = FVector::ZeroVector;
	float ChargeLength = 0.f;
	EEldoriaDeityState PendingStateAfterRecover = EEldoriaDeityState::Alerted;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> HitDuringCharge;
};
