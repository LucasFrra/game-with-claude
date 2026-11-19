// Project Eldoria

#include "Effects/EldoriaCameraShakes.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UEldoriaCameraShake::UEldoriaCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Pattern = CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(TEXT("Pattern"));
	SetRootShakePattern(Pattern);
	bSingleInstance = false;
}

void UEldoriaCameraShake::Configure(float Duration, float LocationAmplitude, float LocationFrequency, float RotationAmplitude, float RotationFrequency, float BlendOut)
{
	Pattern->Duration = Duration;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = BlendOut;
	Pattern->LocationAmplitudeMultiplier = LocationAmplitude;
	Pattern->LocationFrequencyMultiplier = LocationFrequency;
	Pattern->RotationAmplitudeMultiplier = RotationAmplitude;
	Pattern->RotationFrequencyMultiplier = RotationFrequency;
	// mostly vertical motion: things that are heavy shake the ground
	auto Set = [](FPerlinNoiseShaker& Shaker, float Amplitude, float Frequency)
	{
		Shaker.Amplitude = Amplitude;
		Shaker.Frequency = Frequency;
	};
	Set(Pattern->X, 0.3f, 1.f);
	Set(Pattern->Y, 0.3f, 1.1f);
	Set(Pattern->Z, 1.f, 0.9f);
	Set(Pattern->Pitch, 1.f, 1.f);
	Set(Pattern->Yaw, 0.35f, 1.2f);
	Set(Pattern->Roll, 0.5f, 0.8f);
	Set(Pattern->FOV, 0.f, 1.f);
}

UEldoriaShakeFootstep::UEldoriaShakeFootstep(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Configure(0.45f, 6.f, 9.f, 0.6f, 8.f, 0.3f);
}

UEldoriaShakeImpact::UEldoriaShakeImpact(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Configure(1.1f, 16.f, 12.f, 1.6f, 10.f, 0.7f);
}

UEldoriaShakeHit::UEldoriaShakeHit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Configure(0.35f, 7.f, 20.f, 1.4f, 18.f, 0.2f);
}

UEldoriaShakeEffort::UEldoriaShakeEffort(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Configure(0.25f, 1.2f, 10.f, 0.15f, 8.f, 0.15f);
}
