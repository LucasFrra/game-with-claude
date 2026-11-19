// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EldoriaImpactReceiver.generated.h"

/**
 * A physical impact. Strength is a momentum in kg*m/s (mass * speed), so every source
 * speaks the same language: a rolling log, a block falling from a ledge, a charging deity...
 *
 * Reference values:  pushed block ~200 | falling block (10 m) ~15000 | Tharum charge ~60000
 */
USTRUCT(BlueprintType)
struct FEldoriaImpact
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
	float Strength = 0.f;

	/** Normalized direction of the impact (where the receiver is pushed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
	TObjectPtr<AActor> Instigator = nullptr;
};

/** Anything that reacts to physical impacts: collapsible ruins, trees, physics objects, deities... */
UINTERFACE(MinimalAPI, BlueprintType)
class UEldoriaImpactReceiver : public UInterface
{
	GENERATED_BODY()
};

class IEldoriaImpactReceiver
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Impact")
	void ReceiveImpact(const FEldoriaImpact& Impact);

	virtual void ReceiveImpact_Implementation(const FEldoriaImpact& Impact) {}

	/** Sends the impact to Target if it implements the interface. Returns true if it did. */
	static bool SendImpact(UObject* Target, const FEldoriaImpact& Impact)
	{
		if (IsValid(Target) && Target->Implements<UEldoriaImpactReceiver>())
		{
			Execute_ReceiveImpact(Target, Impact);
			return true;
		}
		return false;
	}
};
