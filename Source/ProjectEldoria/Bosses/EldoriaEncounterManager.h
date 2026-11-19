// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EldoriaEncounterManager.generated.h"

class AEldoriaDeityBase;
class AEldoriaControlDevice;
class UEldoriaAbility;
class APostProcessVolume;
class AExponentialHeightFog;
class ADirectionalLight;

/**
 * Runs a deity encounter from start to end:
 *   - intro message, boss status (control remaining) while the player is in the arena,
 *   - reacts to the control device being dismantled,
 *   - liberation: the deity is freed, the world gets its colours and light back,
 *     the deity teaches its ability, closing message.
 * Purely event driven (a slow timer for the arena check).
 */
UCLASS(Blueprintable)
class AEldoriaEncounterManager : public AActor
{
	GENERATED_BODY()

public:

	AEldoriaEncounterManager();

	UFUNCTION(BlueprintPure, Category="Encounter")
	bool IsCompleted() const { return bCompleted; }

protected:

	virtual void BeginPlay() override;

	void Poll();
	void BindDevice(AEldoriaControlDevice* InDevice);
	void UpdateWorldMood(float Alpha);
	void StepRecovery();

	UFUNCTION()
	void HandleLockReleased(AEldoriaControlDevice* InDevice, int32 Released, int32 Total);

	UFUNCTION()
	void HandleDeviceDisabled(AEldoriaControlDevice* InDevice);

	UPROPERTY(EditInstanceOnly, Category="Encounter")
	TObjectPtr<AEldoriaDeityBase> Deity;

	UPROPERTY(EditAnywhere, Category="Encounter")
	TSubclassOf<UEldoriaAbility> RewardAbility;

	UPROPERTY(EditAnywhere, Category="Encounter", meta=(Units="cm"))
	float ArenaRadius = 6500.f;

	UPROPERTY(EditAnywhere, Category="Encounter|Text")
	FText BossName;

	UPROPERTY(EditAnywhere, Category="Encounter|Text")
	FText BossLabel;

	UPROPERTY(EditAnywhere, Category="Encounter|Text", meta=(MultiLine=true))
	FText IntroText;

	UPROPERTY(EditAnywhere, Category="Encounter|Text", meta=(MultiLine=true))
	FText ArenaText;

	UPROPERTY(EditAnywhere, Category="Encounter|Text", meta=(MultiLine=true))
	FText LiberationText;

	UPROPERTY(EditAnywhere, Category="Encounter|Text", meta=(MultiLine=true))
	FText EpilogueText;

	// --- World mood: dulled while the deity is controlled, restored when it is freed ---
	UPROPERTY(EditInstanceOnly, Category="Encounter|World")
	TObjectPtr<APostProcessVolume> PostProcess;

	UPROPERTY(EditInstanceOnly, Category="Encounter|World")
	TObjectPtr<AExponentialHeightFog> Fog;

	UPROPERTY(EditInstanceOnly, Category="Encounter|World")
	TObjectPtr<ADirectionalLight> Sun;

	UPROPERTY(EditAnywhere, Category="Encounter|World")
	float ControlledSaturation = 0.82f;

	UPROPERTY(EditAnywhere, Category="Encounter|World")
	float FreedSaturation = 1.1f;

	UPROPERTY(EditAnywhere, Category="Encounter|World")
	float ControlledFogDensity = 0.026f;

	UPROPERTY(EditAnywhere, Category="Encounter|World")
	float FreedFogDensity = 0.012f;

	UPROPERTY(EditAnywhere, Category="Encounter|World")
	float ControlledSunIntensity = 5.f;

	UPROPERTY(EditAnywhere, Category="Encounter|World")
	float FreedSunIntensity = 7.5f;

	UPROPERTY(EditAnywhere, Category="Encounter|World", meta=(Units="s"))
	float RecoveryDuration = 8.f;

private:

	UPROPERTY(Transient)
	TObjectPtr<AEldoriaControlDevice> Device;

	FTimerHandle PollTimer;
	FTimerHandle RecoveryTimer;
	float RecoveryAlpha = 0.f;
	bool bPlayerInArena = false;
	bool bArenaAnnounced = false;
	bool bCompleted = false;
};
