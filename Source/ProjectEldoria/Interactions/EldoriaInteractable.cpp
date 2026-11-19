// Project Eldoria

#include "Interactions/EldoriaInteractable.h"

#define LOCTEXT_NAMESPACE "EldoriaInteractable"

bool IEldoriaInteractable::CanInteract_Implementation(AActor* Interactor) const
{
	return true;
}

FText IEldoriaInteractable::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return LOCTEXT("DefaultPrompt", "Interact");
}

void IEldoriaInteractable::BeginInteract_Implementation(AActor* Interactor)
{
}

void IEldoriaInteractable::EndInteract_Implementation(AActor* Interactor)
{
}

void IEldoriaInteractable::SetInteractionFocus_Implementation(AActor* Interactor, bool bFocused)
{
}

#undef LOCTEXT_NAMESPACE
