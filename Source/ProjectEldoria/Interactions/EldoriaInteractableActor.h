// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactions/EldoriaInteractable.h"
#include "EldoriaInteractableActor.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEldoriaInteractedSignature, AEldoriaInteractableActor*, Interactable, AActor*, Interactor);

/**
 * Generic interactable actor, configurable per instance.
 *
 * Out of the box it is an "inspectable" object (ruins, carvings, the deity's device...):
 * interacting shows InspectionText on the HUD. Blueprints can extend it through
 * ReceiveInteract / OnInteracted, and C++ mechanisms can derive from it.
 */
UCLASS(Blueprintable)
class AEldoriaInteractableActor : public AActor, public IEldoriaInteractable
{
	GENERATED_BODY()

public:

	AEldoriaInteractableActor();

	// IEldoriaInteractable
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual void BeginInteract_Implementation(AActor* Interactor) override;
	virtual void SetInteractionFocus_Implementation(AActor* Interactor, bool bFocused) override;

	UFUNCTION(BlueprintCallable, Category="Interaction")
	void SetInteractionEnabled(bool bEnabled) { bInteractionEnabled = bEnabled; }

	/** Fired after every successful interaction. */
	UPROPERTY(BlueprintAssignable, Category="Interaction")
	FEldoriaInteractedSignature OnInteracted;

protected:

	UFUNCTION(BlueprintImplementableEvent, Category="Interaction", meta=(DisplayName="On Interact"))
	void ReceiveInteract(AActor* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category="Interaction", meta=(DisplayName="On Focus Changed"))
	void ReceiveFocusChanged(AActor* Interactor, bool bFocused);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	bool bInteractionEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
	FText InteractionPrompt = NSLOCTEXT("EldoriaInteractable", "Inspect", "Inspect");

	/** Shown on screen when interacted with. Leave empty for no message. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(MultiLine=true))
	FText InspectionText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin=0.5, Units="s"))
	float MessageDuration = 5.f;
};
