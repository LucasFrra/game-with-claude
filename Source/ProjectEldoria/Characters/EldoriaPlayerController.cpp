// Project Eldoria

#include "Characters/EldoriaPlayerController.h"
#include "Components/EldoriaInteractionComponent.h"
#include "Components/EldoriaPushComponent.h"
#include "Environment/EldoriaCollapsibleStructure.h"
#include "Bosses/EldoriaDeityBase.h"
#include "Bosses/EldoriaControlDevice.h"
#include "EngineUtils.h"
#include "UI/EldoriaHUDWidget.h"
#include "GameFramework/Pawn.h"
#include "Components/EldoriaAbilityComponent.h"
#include "ProjectEldoria.h"

AEldoriaPlayerController::AEldoriaPlayerController()
{
	HUDWidgetClass = UEldoriaHUDWidget::StaticClass();
}

void AEldoriaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController() && HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UEldoriaHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToPlayerScreen(0);
		}
	}

	// The pawn may have been possessed before BeginPlay
	BindToPawn(GetPawn());
}

void AEldoriaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BindToPawn(InPawn);
}

void AEldoriaPlayerController::OnUnPossess()
{
	UnbindFromPawn();
	Super::OnUnPossess();
}

void AEldoriaPlayerController::BindToPawn(APawn* InPawn)
{
	UEldoriaInteractionComponent* Interaction = InPawn ? InPawn->FindComponentByClass<UEldoriaInteractionComponent>() : nullptr;
	if (Interaction == BoundInteraction.Get())
	{
		return;
	}

	UnbindFromPawn();

	if (Interaction)
	{
		Interaction->OnFocusChanged.AddUniqueDynamic(this, &AEldoriaPlayerController::HandleInteractionFocusChanged);
		BoundInteraction = Interaction;
	}
}

void AEldoriaPlayerController::UnbindFromPawn()
{
	if (UEldoriaInteractionComponent* Interaction = BoundInteraction.Get())
	{
		Interaction->OnFocusChanged.RemoveDynamic(this, &AEldoriaPlayerController::HandleInteractionFocusChanged);
	}
	BoundInteraction.Reset();

	if (HUDWidget)
	{
		HUDWidget->SetInteractionPrompt(FText::GetEmpty());
	}
}

void AEldoriaPlayerController::EldoriaCollapseAll()
{
#if !UE_BUILD_SHIPPING
	for (TActorIterator<AEldoriaCollapsibleStructure> It(GetWorld()); It; ++It)
	{
		It->DebugCollapse();
	}
#endif
}

void AEldoriaPlayerController::EldoriaSetStrength(float NewStrength)
{
#if !UE_BUILD_SHIPPING
	if (UEldoriaPushComponent* Push = GetPawn() ? GetPawn()->FindComponentByClass<UEldoriaPushComponent>() : nullptr)
	{
		Push->SetStrength(NewStrength);
	}
#endif
}

void AEldoriaPlayerController::EldoriaDeityState(int32 StateIndex)
{
#if !UE_BUILD_SHIPPING
	const EEldoriaDeityState NewState = static_cast<EEldoriaDeityState>(FMath::Clamp(StateIndex, 0, static_cast<int32>(EEldoriaDeityState::Liberated)));
	for (TActorIterator<AEldoriaDeityBase> It(GetWorld()); It; ++It)
	{
		It->SetState(NewState);
	}
#endif
}

void AEldoriaPlayerController::EldoriaAdvanceDevice()
{
#if !UE_BUILD_SHIPPING
	for (TActorIterator<AEldoriaControlDevice> It(GetWorld()); It; ++It)
	{
		It->AdvanceSequence(GetPawn());
	}
#endif
}

void AEldoriaPlayerController::EldoriaUseAbility()
{
#if !UE_BUILD_SHIPPING
	if (UEldoriaAbilityComponent* Abilities = GetPawn() ? GetPawn()->FindComponentByClass<UEldoriaAbilityComponent>() : nullptr)
	{
		UE_LOG(LogProjectEldoria, Log, TEXT("[Ability] debug use -> %s"), Abilities->ActivatePrimary() ? TEXT("activated") : TEXT("nothing"));
	}
#endif
}

void AEldoriaPlayerController::EldoriaHeavyStun()
{
#if !UE_BUILD_SHIPPING
	for (TActorIterator<AEldoriaDeityBase> It(GetWorld()); It; ++It)
	{
		It->Stun(16.f);	// HeavyStunDuration default
	}
#endif
}

void AEldoriaPlayerController::EldoriaTeleport(float X, float Y, float Z, float Yaw)
{
#if !UE_BUILD_SHIPPING
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->TeleportTo(FVector(X, Y, Z) * 100.f, FRotator(0.f, Yaw, 0.f));
		SetControlRotation(FRotator(-10.f, Yaw, 0.f));
	}
#endif
}

void AEldoriaPlayerController::HandleInteractionFocusChanged(UObject* FocusedTarget, const FText& Prompt)
{
	if (HUDWidget)
	{
		HUDWidget->SetInteractionPrompt(FocusedTarget ? Prompt : FText::GetEmpty());
	}
}
