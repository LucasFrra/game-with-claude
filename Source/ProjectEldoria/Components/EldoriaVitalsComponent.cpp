// Project Eldoria

#include "Components/EldoriaVitalsComponent.h"
#include "Engine/World.h"
#include "ProjectEldoria.h"

UEldoriaVitalsComponent::UEldoriaVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UEldoriaVitalsComponent::BeginPlay()
{
	Super::BeginPlay();
	Revive();
}

void UEldoriaVitalsComponent::Revive()
{
	HealthQuarters = GetMaxQuarters();
	Stamina = 1.f;
	bExhausted = false;
	OnHealthChanged.Broadcast(HealthQuarters, GetMaxQuarters());
	OnStaminaChanged.Broadcast(Stamina, bExhausted);
}

void UEldoriaVitalsComponent::ApplyDamage(int32 Quarters, AActor* Source)
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Quarters <= 0 || IsDead() || Now < InvulnerableUntil)
	{
		return;
	}
	InvulnerableUntil = Now + InvulnerabilityTime;
	HealthQuarters = FMath::Max(0, HealthQuarters - Quarters);
	UE_LOG(LogProjectEldoria, Log, TEXT("[Vitals] %s takes %d quarter(s) from %s -> %d/%d"), *GetNameSafe(GetOwner()), Quarters, *GetNameSafe(Source), HealthQuarters, GetMaxQuarters());
	OnDamaged.Broadcast(Quarters);
	OnHealthChanged.Broadcast(HealthQuarters, GetMaxQuarters());
	if (IsDead())
	{
		OnDied.Broadcast();
	}
}

void UEldoriaVitalsComponent::Heal(int32 Quarters)
{
	if (IsDead())
	{
		return;
	}
	HealthQuarters = FMath::Min(GetMaxQuarters(), HealthQuarters + Quarters);
	OnHealthChanged.Broadcast(HealthQuarters, GetMaxQuarters());
}

bool UEldoriaVitalsComponent::ConsumeStamina(float Amount)
{
	if (bExhausted)
	{
		return false;
	}
	SinceDrain = 0.f;
	Stamina = FMath::Max(0.f, Stamina - Amount);
	if (Stamina <= 0.f)
	{
		bExhausted = true;
	}
	OnStaminaChanged.Broadcast(Stamina, bExhausted);
	return !bExhausted;
}

void UEldoriaVitalsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SinceDrain += DeltaTime;
	if (Stamina < 1.f && SinceDrain >= StaminaRegenDelay)
	{
		// Exhausted lungs refill a little slower
		Stamina = FMath::Min(1.f, Stamina + StaminaRegenRate * (bExhausted ? 0.75f : 1.f) * DeltaTime);
		if (bExhausted && Stamina >= 1.f)
		{
			bExhausted = false;
		}
		OnStaminaChanged.Broadcast(Stamina, bExhausted);
	}
}
