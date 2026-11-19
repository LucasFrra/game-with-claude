// Project Eldoria

#include "Bosses/EldoriaEncounterManager.h"
#include "Characters/EldoriaCharacter.h"
#include "Bosses/EldoriaControlDevice.h"
#include "Bosses/EldoriaDeityBase.h"
#include "Bosses/EldoriaTharum.h"
#include "Abilities/EldoriaAbility.h"
#include "Characters/EldoriaPlayerController.h"
#include "Components/EldoriaAbilityComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/EldoriaHUDWidget.h"
#include "UI/EldoriaMessageSubsystem.h"

#define LOCTEXT_NAMESPACE "EldoriaEncounter"

AEldoriaEncounterManager::AEldoriaEncounterManager()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	RewardAbility = UEldoriaAbility_Force::StaticClass();
	BossName = LOCTEXT("BossName", "THARUM");
	BossLabel = LOCTEXT("BossLabel", "Divinité de la Jungle  -  emprise du mécanisme");
	IntroText = LOCTEXT("Intro", "Un grondement sourd fait trembler le sol. Quelque chose d'immense vit dans cette jungle.");
	ArenaText = LOCTEXT("Arena", "Tharum. Un mécanisme est vissé dans son dos... Il faudrait l'observer avant d'agir.");
	LiberationText = LOCTEXT("Liberation", "Le mécanisme se tait. Tharum s'immobilise, puis s'incline lentement.");
	EpilogueText = LOCTEXT("Epilogue", "La jungle respire à nouveau. Ce qui était inébranlable ne l'est plus : un passage au nord attend peut-être d'être ouvert.");
}

void AEldoriaEncounterManager::BeginPlay()
{
	Super::BeginPlay();

	if (!Deity)
	{
		for (TActorIterator<AEldoriaDeityBase> It(GetWorld()); It; ++It)
		{
			Deity = *It;
			break;
		}
	}

	UpdateWorldMood(0.f);
	GetWorld()->GetTimerManager().SetTimer(PollTimer, this, &AEldoriaEncounterManager::Poll, 0.25f, true, 0.5f);

	// The HUD is created by the player controller: give it a moment before the first message
	FTimerHandle IntroTimer;
	GetWorld()->GetTimerManager().SetTimer(IntroTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UEldoriaMessageSubsystem::ShowMessage(this, IntroText, 6.f);
	}), 1.5f, false);
}

void AEldoriaEncounterManager::Poll()
{
	// The device is spawned by the deity at BeginPlay: bind as soon as it exists
	if (!Device)
	{
		if (AEldoriaTharum* Tharum = Cast<AEldoriaTharum>(Deity))
		{
			BindDevice(Tharum->GetControlDevice());
		}
	}

	const ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	AEldoriaPlayerController* Controller = Cast<AEldoriaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	UEldoriaHUDWidget* HUD = Controller ? Controller->GetHUDWidget() : nullptr;
	if (!Player || !Deity || !HUD)
	{
		return;
	}

	const bool bInArena = !bCompleted && FVector::Dist2D(Player->GetActorLocation(), Deity->GetActorLocation()) < ArenaRadius;
	if (bInArena && !bArenaAnnounced)
	{
		bArenaAnnounced = true;
		UEldoriaMessageSubsystem::ShowMessage(this, ArenaText, 6.f);
		// first sight of the deity: the camera turns to it, its name is carved on screen
		if (AEldoriaCharacter* Hero = Cast<AEldoriaCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Hero->FocusOn(Deity, 2.8f);
		}
		HUD->ShowBanner(BossName, LOCTEXT("Epithet", "Divinité de la Jungle"), 3.5f);
	}
	if (bInArena || bPlayerInArena)
	{
		HUD->SetBossStatus(bInArena, BossName, BossLabel, Device ? Device->GetInstability() : 0.f);
	}
	bPlayerInArena = bInArena;
}

void AEldoriaEncounterManager::BindDevice(AEldoriaControlDevice* InDevice)
{
	if (!InDevice || Device == InDevice)
	{
		return;
	}
	Device = InDevice;
	Device->OnLockReleased.AddDynamic(this, &AEldoriaEncounterManager::HandleLockReleased);
	Device->OnDeviceDisabled.AddDynamic(this, &AEldoriaEncounterManager::HandleDeviceDisabled);
}

void AEldoriaEncounterManager::HandleLockReleased(AEldoriaControlDevice* InDevice, int32 Released, int32 Total)
{
	// The grip loosens: the world brightens a little with each lock
	UpdateWorldMood(0.25f * Released / FMath::Max(Total, 1));
}

void AEldoriaEncounterManager::HandleDeviceDisabled(AEldoriaControlDevice* InDevice)
{
	if (bCompleted)
	{
		return;
	}
	bCompleted = true;

	if (Deity)
	{
		Deity->Liberate();
		if (AEldoriaCharacter* Hero = Cast<AEldoriaCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Hero->FocusOn(Deity, 3.5f);
		}
	}
	UEldoriaMessageSubsystem::ShowMessage(this, LiberationText, 6.f);

	AEldoriaPlayerController* Controller = Cast<AEldoriaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (UEldoriaHUDWidget* HUD = Controller ? Controller->GetHUDWidget() : nullptr)
	{
		HUD->SetBossStatus(false, BossName, BossLabel, 1.f);
	}

	// Recovery of the world, then the reward
	RecoveryAlpha = 0.25f;
	GetWorld()->GetTimerManager().SetTimer(RecoveryTimer, this, &AEldoriaEncounterManager::StepRecovery, 0.05f, true);

	FTimerHandle RewardTimer;
	GetWorld()->GetTimerManager().SetTimer(RewardTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
		UEldoriaAbilityComponent* Abilities = Player ? Player->FindComponentByClass<UEldoriaAbilityComponent>() : nullptr;
		UEldoriaAbility* Ability = Abilities ? Abilities->GrantAbility(RewardAbility) : nullptr;

		AEldoriaPlayerController* PC = Cast<AEldoriaPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
		if (UEldoriaHUDWidget* HUD = PC ? PC->GetHUDWidget() : nullptr)
		{
			const FText Title = Ability ? Ability->GetDisplayName() : LOCTEXT("Gift", "Un don");
			const FText Subtitle = Ability ? Ability->GetDescription() : FText::GetEmpty();
			HUD->ShowBanner(FText::Format(LOCTEXT("Learnt", "Tharum vous enseigne : {0}"), Title), Subtitle, 7.f);
		}

		FTimerHandle EpilogueTimer;
		GetWorld()->GetTimerManager().SetTimer(EpilogueTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UEldoriaMessageSubsystem::ShowMessage(this, EpilogueText, 8.f);
		}), 8.f, false);
	}), 6.f, false);
}

void AEldoriaEncounterManager::StepRecovery()
{
	RecoveryAlpha = FMath::Min(1.f, RecoveryAlpha + 0.05f / FMath::Max(RecoveryDuration, 0.1f));
	UpdateWorldMood(RecoveryAlpha);
	if (RecoveryAlpha >= 1.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(RecoveryTimer);
	}
}

void AEldoriaEncounterManager::UpdateWorldMood(float Alpha)
{
	const float A = FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	if (PostProcess)
	{
		const float Saturation = FMath::Lerp(ControlledSaturation, FreedSaturation, A);
		PostProcess->Settings.bOverride_ColorSaturation = true;
		PostProcess->Settings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	}
	if (Fog && Fog->GetComponent())
	{
		Fog->GetComponent()->SetFogDensity(FMath::Lerp(ControlledFogDensity, FreedFogDensity, A));
	}
	if (Sun && Sun->GetLightComponent())
	{
		Sun->GetLightComponent()->SetIntensity(FMath::Lerp(ControlledSunIntensity, FreedSunIntensity, A));
	}
}

#undef LOCTEXT_NAMESPACE
