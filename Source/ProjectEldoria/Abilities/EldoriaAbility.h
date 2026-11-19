// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EldoriaAbility.generated.h"

class ACharacter;

/**
 * A capability taught by a deity. Abilities are data-configurable objects owned by the
 * UEldoriaAbilityComponent: they can change the character permanently when granted (passive part)
 * and do something when the ability input is pressed (active part).
 * They must expand what the player can do with the world, never act as a single-use key.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UEldoriaAbility : public UObject
{
	GENERATED_BODY()

public:

	/** Called once when the ability is learnt. */
	virtual void OnGranted(ACharacter* Owner);

	/** Ability input pressed. Returns true if something happened. */
	virtual bool Activate(ACharacter* Owner);

	UFUNCTION(BlueprintPure, Category="Ability")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category="Ability")
	FText GetDescription() const { return Description; }

protected:

	UFUNCTION(BlueprintImplementableEvent, Category="Ability", meta=(DisplayName="On Granted"))
	void ReceiveGranted(ACharacter* Owner);

	UFUNCTION(BlueprintImplementableEvent, Category="Ability", meta=(DisplayName="On Activated"))
	void ReceiveActivated(ACharacter* Owner);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(MultiLine=true))
	FText Description;

	/** Seconds between two activations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ability", meta=(ClampMin=0, Units="s"))
	float Cooldown = 0.8f;

	double LastActivationTime = -1000.0;
};

/**
 * FORCE - Tharum's strength.
 * Passive: raises the character's physical strength, so heavier objects can be carried, dragged
 * and pushed (the same objects, new possibilities). Active: a powerful shove that sends whatever
 * stands in front of the character tumbling, scaled by its mass.
 */
UCLASS(meta=(DisplayName="Force"))
class UEldoriaAbility_Force : public UEldoriaAbility
{
	GENERATED_BODY()

public:

	UEldoriaAbility_Force();

	virtual void OnGranted(ACharacter* Owner) override;
	virtual bool Activate(ACharacter* Owner) override;

protected:

	/** Strength after learning FORCE (base character strength is 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Force", meta=(ClampMin=1))
	float Strength = 3.f;

	/** Momentum of the shove, in kg*m/s. A 1 t block flies at ~ this / 1000 m/s. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Force", meta=(ClampMin=0))
	float ShoveMomentum = 9000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Force", meta=(ClampMin=50, Units="cm"))
	float ShoveRange = 260.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Force", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float MaxShoveSpeed = 1200.f;

	/** Objects that only FORCE can move (RequiredStrength above 1, within Strength) get at least this speed:
	 *  the shove has to visibly budge what was immovable before. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Force", meta=(ClampMin=0, Units="CentimetersPerSecond"))
	float MinUnlockedShoveSpeed = 700.f;
};
