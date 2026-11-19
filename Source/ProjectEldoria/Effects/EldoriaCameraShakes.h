// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "EldoriaCameraShakes.generated.h"

class UPerlinNoiseCameraShakePattern;

/** Base: a Perlin-noise shake configured in code (no assets needed). */
UCLASS(Abstract)
class UEldoriaCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()

public:

	UEldoriaCameraShake(const FObjectInitializer& ObjectInitializer);

protected:

	void Configure(float Duration, float LocationAmplitude, float LocationFrequency, float RotationAmplitude, float RotationFrequency, float BlendOut);

	UPROPERTY()
	TObjectPtr<UPerlinNoiseCameraShakePattern> Pattern;
};

/** A giant's footfall: short, low, heavy thump. */
UCLASS()
class UEldoriaShakeFootstep : public UEldoriaCameraShake
{
	GENERATED_BODY()
public:
	UEldoriaShakeFootstep(const FObjectInitializer& ObjectInitializer);
};

/** Something massive hits something: tower falling, charge into a wall. */
UCLASS()
class UEldoriaShakeImpact : public UEldoriaCameraShake
{
	GENERATED_BODY()
public:
	UEldoriaShakeImpact(const FObjectInitializer& ObjectInitializer);
};

/** The player gets hit. */
UCLASS()
class UEldoriaShakeHit : public UEldoriaCameraShake
{
	GENERATED_BODY()
public:
	UEldoriaShakeHit(const FObjectInitializer& ObjectInitializer);
};

/** Straining: pushing something heavy, a small tremor per step. */
UCLASS()
class UEldoriaShakeEffort : public UEldoriaCameraShake
{
	GENERATED_BODY()
public:
	UEldoriaShakeEffort(const FObjectInitializer& ObjectInitializer);
};
