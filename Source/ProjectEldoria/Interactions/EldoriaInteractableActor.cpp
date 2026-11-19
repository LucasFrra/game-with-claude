// Project Eldoria

#include "Interactions/EldoriaInteractableActor.h"
#include "UI/EldoriaMessageSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

AEldoriaInteractableActor::AEldoriaInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RootComponent = Mesh;
}

bool AEldoriaInteractableActor::CanInteract_Implementation(AActor* Interactor) const
{
	return bInteractionEnabled;
}

FText AEldoriaInteractableActor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return InteractionPrompt;
}

void AEldoriaInteractableActor::BeginInteract_Implementation(AActor* Interactor)
{
	if (!InspectionText.IsEmpty())
	{
		UEldoriaMessageSubsystem::ShowMessage(this, InspectionText, MessageDuration);
	}

	ReceiveInteract(Interactor);
	OnInteracted.Broadcast(this, Interactor);
}

void AEldoriaInteractableActor::SetInteractionFocus_Implementation(AActor* Interactor, bool bFocused)
{
	ReceiveFocusChanged(Interactor, bFocused);
}
