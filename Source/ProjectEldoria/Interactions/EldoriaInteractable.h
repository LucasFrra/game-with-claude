// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "EldoriaInteractable.generated.h"

/**
 * Anything the player can interact with: levers, inspectable ruins, pushable blocks,
 * boss interaction points...
 *
 * Can be implemented by an Actor OR by a component (useful for large creatures that
 * expose several interaction points). The interaction component checks the hit
 * component first, then its owning actor.
 *
 * Interaction model:
 *  - BeginInteract is sent when the interact input is pressed.
 *  - EndInteract is sent when it is released (or when focus is lost while held).
 *  Instant interactions (levers, inspection) only need BeginInteract.
 *  Held interactions (pushing, cranking) can use both.
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UEldoriaInteractable : public UInterface
{
	GENERATED_BODY()
};

class IEldoriaInteractable
{
	GENERATED_BODY()

public:

	/** Whether the interactor can currently interact with this object. Hidden from the prompt if false. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	bool CanInteract(AActor* Interactor) const;

	/** Short contextual verb shown to the player, e.g. "Pull lever", "Inspect". */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	FText GetInteractionPrompt(AActor* Interactor) const;

	/** Interact input pressed while this object is focused. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void BeginInteract(AActor* Interactor);

	/** Interact input released, or focus lost during a held interaction. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void EndInteract(AActor* Interactor);

	/** This object became (or stopped being) the interactor's current focus. Useful for highlights. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Interaction")
	void SetInteractionFocus(AActor* Interactor, bool bFocused);

	// Default native implementations (declared explicitly, otherwise UHT generates
	// stubs where CanInteract returns false).
	virtual bool CanInteract_Implementation(AActor* Interactor) const;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const;
	virtual void BeginInteract_Implementation(AActor* Interactor);
	virtual void EndInteract_Implementation(AActor* Interactor);
	virtual void SetInteractionFocus_Implementation(AActor* Interactor, bool bFocused);
};
