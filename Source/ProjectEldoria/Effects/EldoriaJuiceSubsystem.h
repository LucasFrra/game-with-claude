// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Camera/CameraShakeBase.h"
#include "EldoriaJuiceSubsystem.generated.h"

/**
 * "Game feel" helpers used everywhere: hit-stop, slow motion, distance-attenuated camera shakes,
 * gamepad rumble and dust puffs. Time effects run on real time, so they are not slowed by themselves.
 */
UCLASS()
class UEldoriaJuiceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	static UEldoriaJuiceSubsystem* Get(const UObject* WorldContext);

	/** Near-freeze for a few frames on a strong hit. */
	UFUNCTION(BlueprintCallable, Category="Juice")
	void HitStop(float RealSeconds = 0.07f, float Dilation = 0.03f);

	/** Dramatic slow motion (eases back to normal speed). */
	UFUNCTION(BlueprintCallable, Category="Juice")
	void SlowMotion(float RealSeconds = 1.2f, float Dilation = 0.35f);

	/** Camera shake for the local player, full strength within Inner, fading to nothing at Outer. */
	void Shake(TSubclassOf<UCameraShakeBase> ShakeClass, const FVector& Epicenter, float Inner, float Outer, float Scale = 1.f);

	UFUNCTION(BlueprintCallable, Category="Juice")
	void Rumble(float Intensity, float Duration);

	/** Rumble attenuated with the distance to the player. */
	void RumbleAt(const FVector& Epicenter, float Inner, float Outer, float Intensity, float Duration);

	/** A few cartoon dust clouds billowing out from Location. */
	UFUNCTION(BlueprintCallable, Category="Juice")
	void Dust(const FVector& Location, float Size = 1.f, int32 Puffs = 6);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UEldoriaJuiceSubsystem, STATGROUP_Tickables); }

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:

	float Attenuation(const FVector& Epicenter, float Inner, float Outer) const;

	double HitStopUntil = 0.0;
	float HitStopDilation = 1.f;
	double SlowStart = 0.0;
	double SlowUntil = 0.0;
	float SlowDilation = 1.f;
	bool bDilated = false;
};
