// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Interactions/EldoriaInteractable.h"
#include "EldoriaInteractableComponent.generated.h"

class UEldoriaInteractableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaComponentInteracted, UEldoriaInteractableComponent*, Component, AActor*, Interactor);

/**
 * A mesh the player can interact with directly: a lock on a mechanism, a handle, a switch...
 * Lets one actor expose several independent interaction points (the interaction component
 * prefers an interactable component over its owning actor).
 */
UCLASS(ClassGroup=(Eldoria), meta=(BlueprintSpawnableComponent))
class UEldoriaInteractableComponent : public UStaticMeshComponent, public IEldoriaInteractable
{
	GENERATED_BODY()

public:

	UEldoriaInteractableComponent();

	virtual bool CanInteract_Implementation(AActor* Interactor) const override { return bInteractionEnabled; }
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override { return Prompt; }
	virtual void BeginInteract_Implementation(AActor* Interactor) override { OnInteracted.Broadcast(this, Interactor); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText Prompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bInteractionEnabled = true;

	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FEldoriaComponentInteracted OnInteracted;
};
