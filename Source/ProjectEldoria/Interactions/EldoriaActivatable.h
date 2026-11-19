// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EldoriaActivatable.generated.h"

/**
 * Something a mechanism (lever, pressure plate, ...) can drive: lifts, gates, bridges...
 * Decouples "what triggers" from "what reacts" so any trigger can drive any target.
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UEldoriaActivatable : public UInterface
{
	GENERATED_BODY()
};

class IEldoriaActivatable
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mechanism")
	void SetActivated(bool bActivated, AActor* Instigator);

	virtual void SetActivated_Implementation(bool bActivated, AActor* Instigator) {}
};
