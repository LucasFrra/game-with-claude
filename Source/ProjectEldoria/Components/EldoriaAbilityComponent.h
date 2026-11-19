// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EldoriaAbilityComponent.generated.h"

class UEldoriaAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEldoriaAbilityGranted, UEldoriaAbility*, Ability);

/** Holds the abilities the character has learnt and routes the ability input to them. */
UCLASS(ClassGroup=(Eldoria), meta=(BlueprintSpawnableComponent))
class UEldoriaAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	/** Learns an ability (ignored if one of the same class is already known). */
	UFUNCTION(BlueprintCallable, Category="Ability")
	UEldoriaAbility* GrantAbility(TSubclassOf<UEldoriaAbility> AbilityClass);

	UFUNCTION(BlueprintPure, Category="Ability")
	bool HasAbility(TSubclassOf<UEldoriaAbility> AbilityClass) const;

	/** Activates the most recently learnt ability. */
	UFUNCTION(BlueprintCallable, Category="Ability")
	bool ActivatePrimary();

	UPROPERTY(BlueprintAssignable, Category="Ability")
	FEldoriaAbilityGranted OnAbilityGranted;

protected:

	/** Abilities known from the start (configurable in the Blueprint). */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category="Ability")
	TArray<TObjectPtr<UEldoriaAbility>> Abilities;

	virtual void BeginPlay() override;
};
