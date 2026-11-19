// Project Eldoria

#include "Components/EldoriaAbilityComponent.h"
#include "Abilities/EldoriaAbility.h"
#include "GameFramework/Character.h"
#include "ProjectEldoria.h"

void UEldoriaAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	for (UEldoriaAbility* Ability : Abilities)
	{
		if (Ability)
		{
			Ability->OnGranted(Cast<ACharacter>(GetOwner()));
		}
	}
}

bool UEldoriaAbilityComponent::HasAbility(TSubclassOf<UEldoriaAbility> AbilityClass) const
{
	return Abilities.ContainsByPredicate([AbilityClass](const UEldoriaAbility* Ability) { return Ability && Ability->IsA(AbilityClass); });
}

UEldoriaAbility* UEldoriaAbilityComponent::GrantAbility(TSubclassOf<UEldoriaAbility> AbilityClass)
{
	if (!AbilityClass || HasAbility(AbilityClass))
	{
		return nullptr;
	}
	UEldoriaAbility* Ability = NewObject<UEldoriaAbility>(this, AbilityClass);
	Abilities.Add(Ability);
	Ability->OnGranted(Cast<ACharacter>(GetOwner()));
	UE_LOG(LogProjectEldoria, Log, TEXT("[Ability] %s granted to %s"), *AbilityClass->GetName(), *GetNameSafe(GetOwner()));
	OnAbilityGranted.Broadcast(Ability);
	return Ability;
}

bool UEldoriaAbilityComponent::ActivatePrimary()
{
	for (int32 Index = Abilities.Num() - 1; Index >= 0; --Index)
	{
		if (Abilities[Index] && Abilities[Index]->Activate(Cast<ACharacter>(GetOwner())))
		{
			return true;
		}
	}
	return false;
}
