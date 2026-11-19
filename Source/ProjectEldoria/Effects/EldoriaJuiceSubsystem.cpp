// Project Eldoria

#include "Effects/EldoriaJuiceSubsystem.h"
#include "Effects/EldoriaDustPuff.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"

UEldoriaJuiceSubsystem* UEldoriaJuiceSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UEldoriaJuiceSubsystem>() : nullptr;
}

bool UEldoriaJuiceSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UEldoriaJuiceSubsystem::HitStop(float RealSeconds, float Dilation)
{
	HitStopUntil = FMath::Max(HitStopUntil, FPlatformTime::Seconds() + RealSeconds);
	HitStopDilation = Dilation;
}

void UEldoriaJuiceSubsystem::SlowMotion(float RealSeconds, float Dilation)
{
	SlowStart = FPlatformTime::Seconds();
	SlowUntil = SlowStart + RealSeconds;
	SlowDilation = Dilation;
}

void UEldoriaJuiceSubsystem::Tick(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	float Dilation = 1.f;
	if (Now < SlowUntil)
	{
		// hold the slow motion, then ease back during the last third
		const float T = float((Now - SlowStart) / FMath::Max(SlowUntil - SlowStart, 0.01));
		Dilation = T < 0.66f ? SlowDilation : FMath::Lerp(SlowDilation, 1.f, (T - 0.66f) / 0.34f);
	}
	if (Now < HitStopUntil)
	{
		Dilation = FMath::Min(Dilation, HitStopDilation);
	}
	if (Dilation < 0.999f || bDilated)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
		bDilated = Dilation < 0.999f;
	}
}

float UEldoriaJuiceSubsystem::Attenuation(const FVector& Epicenter, float Inner, float Outer) const
{
	const APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->PlayerCameraManager)
	{
		return 0.f;
	}
	const float Distance = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), Epicenter);
	return 1.f - FMath::Clamp((Distance - Inner) / FMath::Max(Outer - Inner, 1.f), 0.f, 1.f);
}

void UEldoriaJuiceSubsystem::Shake(TSubclassOf<UCameraShakeBase> ShakeClass, const FVector& Epicenter, float Inner, float Outer, float Scale)
{
	const float A = Attenuation(Epicenter, Inner, Outer) * Scale;
	if (A > 0.02f)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->ClientStartCameraShake(ShakeClass, A);
		}
	}
}

void UEldoriaJuiceSubsystem::Rumble(float Intensity, float Duration)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->PlayDynamicForceFeedback(FMath::Clamp(Intensity, 0.f, 1.f), Duration, true, true, true, true);
	}
}

void UEldoriaJuiceSubsystem::RumbleAt(const FVector& Epicenter, float Inner, float Outer, float Intensity, float Duration)
{
	const float A = Attenuation(Epicenter, Inner, Outer);
	if (A > 0.05f)
	{
		Rumble(Intensity * A, Duration);
	}
}

void UEldoriaJuiceSubsystem::Dust(const FVector& Location, float Size, int32 Puffs)
{
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AEldoriaDustPuff* Puff = World->SpawnActor<AEldoriaDustPuff>(Location, FRotator::ZeroRotator, Params))
		{
			Puff->Init(Size, Puffs);
		}
	}
}
