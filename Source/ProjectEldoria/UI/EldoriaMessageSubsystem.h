// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EldoriaMessageSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaMessageShown, const FText&, Message, float, Duration);

/**
 * Decoupled channel for short on-screen gameplay messages
 * (inspection text, discoveries, ability unlocks...).
 * Gameplay code posts messages here; the HUD listens. Neither knows about the other.
 */
UCLASS()
class UEldoriaMessageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	/** Posts a message for Duration seconds. */
	UFUNCTION(BlueprintCallable, Category="Eldoria|Messages", meta=(WorldContext="WorldContextObject"))
	static void ShowMessage(const UObject* WorldContextObject, const FText& Message, float Duration = 4.f);

	UPROPERTY(BlueprintAssignable, Category="Eldoria|Messages")
	FEldoriaMessageShown OnMessageShown;

protected:

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
