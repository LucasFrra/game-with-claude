// Project Eldoria

#include "Interactions/EldoriaInteractableComponent.h"

UEldoriaInteractableComponent::UEldoriaInteractableComponent()
{
	SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Prompt = NSLOCTEXT("EldoriaInteractableComponent", "Default", "Interagir");
}
