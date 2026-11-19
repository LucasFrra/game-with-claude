// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EldoriaVitalsComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaHealthChanged, int32, Quarters, int32, MaxQuarters);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaStaminaChanged, float, Stamina, bool, bExhausted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEldoriaDamaged, int32, Quarters);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEldoriaDied);

/**
 * Hearts (counted in quarters, like Zelda) and stamina (0..1).
 *
 * Stamina drains while sprinting / climbing and refills after a short pause. Emptying it leaves the
 * character exhausted until it is full again: no sprint, no climbing, slower walk.
 */
UCLASS(ClassGroup=(Eldoria), meta=(BlueprintSpawnableComponent))
class UEldoriaVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UEldoriaVitalsComponent();

	UFUNCTION(BlueprintCallable, Category="Vitals")
	void ApplyDamage(int32 Quarters, AActor* Source);

	UFUNCTION(BlueprintCallable, Category="Vitals")
	void Heal(int32 Quarters);

	/** Full health and stamina (respawn). */
	UFUNCTION(BlueprintCallable, Category="Vitals")
	void Revive();

	/** Continuous drain (Amount per call). Returns false when exhausted: the action must stop. */
	UFUNCTION(BlueprintCallable, Category="Vitals")
	bool ConsumeStamina(float Amount);

	UFUNCTION(BlueprintPure, Category="Vitals") int32 GetHealthQuarters() const { return HealthQuarters; }
	UFUNCTION(BlueprintPure, Category="Vitals") int32 GetMaxQuarters() const { return MaxHearts * 4; }
	UFUNCTION(BlueprintPure, Category="Vitals") float GetStamina() const { return Stamina; }
	UFUNCTION(BlueprintPure, Category="Vitals") bool IsExhausted() const { return bExhausted; }
	UFUNCTION(BlueprintPure, Category="Vitals") bool IsDead() const { return HealthQuarters <= 0; }

	UPROPERTY(BlueprintAssignable, Category="Vitals") FEldoriaHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category="Vitals") FEldoriaStaminaChanged OnStaminaChanged;
	UPROPERTY(BlueprintAssignable, Category="Vitals") FEldoriaDamaged OnDamaged;
	UPROPERTY(BlueprintAssignable, Category="Vitals") FEldoriaDied OnDied;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vitals", meta=(ClampMin=1))
	int32 MaxHearts = 3;

	/** Stamina regained per second once resting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vitals", meta=(ClampMin=0))
	float StaminaRegenRate = 0.32f;

	/** Pause after the last drain before stamina refills. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vitals", meta=(ClampMin=0, Units="s"))
	float StaminaRegenDelay = 0.8f;

	/** No damage taken for this long after a hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vitals", meta=(ClampMin=0, Units="s"))
	float InvulnerabilityTime = 1.2f;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	int32 HealthQuarters = 12;
	float Stamina = 1.f;
	bool bExhausted = false;
	float SinceDrain = 10.f;
	float InvulnerableUntil = -1.f;
};
