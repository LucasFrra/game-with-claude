// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EldoriaDustPuff.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Cartoon dust: a handful of soft round clouds that billow out, rise and fade (BOTW-like puffs).
 * Spawned by UEldoriaJuiceSubsystem::Dust; destroys itself.
 */
UCLASS(NotBlueprintable)
class AEldoriaDustPuff : public AActor
{
	GENERATED_BODY()

public:

	AEldoriaDustPuff();

	void Init(float Size, int32 Puffs);

	virtual void Tick(float DeltaSeconds) override;

private:

	struct FPuff
	{
		TObjectPtr<UStaticMeshComponent> Mesh;
		TObjectPtr<UMaterialInstanceDynamic> Material;
		FVector Velocity;
		float StartScale;
		float EndScale;
	};
	TArray<FPuff> Puffs;
	float Age = 0.f;
	float Life = 1.4f;
};
