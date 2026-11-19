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
#include "Components/EldoriaVitalsComponent.h"
#include "Characters/EldoriaCharacter.h"
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
	if (UEldoriaVitalsComponent* Vitals = InPawn ? InPawn->FindComponentByClass<UEldoriaVitalsComponent>() : nullptr)
	{
		Vitals->OnHealthChanged.AddUniqueDynamic(this, &AEldoriaPlayerController::HandleHealthChanged);
		Vitals->OnStaminaChanged.AddUniqueDynamic(this, &AEldoriaPlayerController::HandleStaminaChanged);
		HandleHealthChanged(Vitals->GetHealthQuarters(), Vitals->GetMaxQuarters());
		HandleStaminaChanged(Vitals->GetStamina(), Vitals->IsExhausted());
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

void AEldoriaPlayerController::EldoriaDamage(int32 Quarters)
{
#if !UE_BUILD_SHIPPING
	if (UEldoriaVitalsComponent* Vitals = GetPawn() ? GetPawn()->FindComponentByClass<UEldoriaVitalsComponent>() : nullptr)
	{
		Vitals->ApplyDamage(Quarters, nullptr);
	}
#endif
}

void AEldoriaPlayerController::EldoriaHoldMove(float X, float Y, float Seconds)
{
#if !UE_BUILD_SHIPPING
	if (AEldoriaCharacter* Hero = Cast<AEldoriaCharacter>(GetPawn()))
	{
		Hero->HoldTestInput(FVector2D(X, Y), Seconds);
	}
#endif
}

void AEldoriaPlayerController::EldoriaJump()
{
#if !UE_BUILD_SHIPPING
	if (AEldoriaCharacter* Hero = Cast<AEldoriaCharacter>(GetPawn()))
	{
		static_cast<AProjectEldoriaCharacter*>(Hero)->DoJumpStart();	// public in the base class
		FTimerHandle Release;
		GetWorldTimerManager().SetTimer(Release, FTimerDelegate::CreateWeakLambda(Hero, [Hero]() { Hero->DoJumpEnd(); }), 0.2f, false);
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

void AEldoriaPlayerController::HandleHealthChanged(int32 Quarters, int32 MaxQuarters)
{
	if (HUDWidget)
	{
		HUDWidget->SetHealth(Quarters, MaxQuarters);
	}
}

void AEldoriaPlayerController::HandleStaminaChanged(float Stamina, bool bExhausted)
{
	if (HUDWidget)
	{
		HUDWidget->SetStamina(Stamina, bExhausted);
	}
}

void AEldoriaPlayerController::HandleInteractionFocusChanged(UObject* FocusedTarget, const FText& Prompt)
{
	if (HUDWidget)
	{
		HUDWidget->SetInteractionPrompt(FocusedTarget ? Prompt : FText::GetEmpty());
	}
}
