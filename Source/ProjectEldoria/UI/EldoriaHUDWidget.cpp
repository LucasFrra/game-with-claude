// Project Eldoria

#include "UI/EldoriaHUDWidget.h"
#include "UI/EldoriaMessageSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace EldoriaUI
{
	// Palette of the art direction (docs/ART_DIRECTION.md)
	const FLinearColor Cream(1.f, 0.955f, 0.86f);
	const FLinearColor Gold(0.98f, 0.80f, 0.42f);
	const FLinearColor Muted(0.82f, 0.80f, 0.74f);
	const FLinearColor Ink(0.09f, 0.08f, 0.07f);
	const FLinearColor Panel(0.015f, 0.02f, 0.03f, 0.62f);
	const FLinearColor Parchment(0.93f, 0.88f, 0.74f, 0.95f);
	const FLinearColor Shadow(0.f, 0.f, 0.f, 0.6f);
}

TSharedRef<SWidget> UEldoriaHUDWidget::RebuildWidget()
{
	// Pure C++ instance (no designer tree): build a default layout
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	return Super::RebuildWidget();
}

FSlateFontInfo UEldoriaHUDWidget::MakeFont(const TCHAR* FaceName, float Size, int32 LetterSpacing)
{
	const FString Path = FString::Printf(TEXT("/Game/Project/UI/Fonts/%s.%s"), FaceName, FaceName);
	UFontFace* Face = LoadObject<UFontFace>(nullptr, *Path);
	if (!Face)
	{
		FSlateFontInfo Fallback = FCoreStyle::GetDefaultFontStyle("Regular", Size);
		Fallback.LetterSpacing = LetterSpacing;
		return Fallback;
	}
	UFont* Font = NewObject<UFont>(this);
	Font->FontCacheType = EFontCacheType::Runtime;
	FTypefaceEntry Entry(TEXT("Regular"));
	Entry.Font = FFontData(Face);
	Font->CompositeFont.DefaultTypeface.Fonts.Add(Entry);
	RuntimeFonts.Add(Font);

	FSlateFontInfo Info(Font, Size, TEXT("Regular"));
	Info.LetterSpacing = LetterSpacing;
	return Info;
}

void UEldoriaHUDWidget::BuildDefaultLayout()
{
	using namespace EldoriaUI;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	const FSlateFontInfo TitleFont = MakeFont(TEXT("MarcellusSC-Regular"), 30, 140);
	const FSlateFontInfo BannerFont = MakeFont(TEXT("MarcellusSC-Regular"), 54, 90);
	const FSlateFontInfo BodyFont = MakeFont(TEXT("AlegreyaSans-Regular"), 22);
	const FSlateFontInfo BoldFont = MakeFont(TEXT("AlegreyaSans-Bold"), 21);
	const FSlateFontInfo ItalicFont = MakeFont(TEXT("AlegreyaSans-Italic"), 17);
	const FSlateFontInfo ItalicLarge = MakeFont(TEXT("AlegreyaSans-Italic"), 25);

	auto MakeText = [this](FName Name, const FSlateFontInfo& Font, const FLinearColor& Color) -> UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetFont(Font);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(Shadow);
		return Text;
	};
	auto Place = [Root](UWidget* Widget, float AnchorY, float OffsetY, const FVector2D& Size = FVector2D::ZeroVector) -> UCanvasPanelSlot*
	{
		UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Widget);
		CanvasSlot->SetAnchors(FAnchors(0.5f, AnchorY));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(0.f, OffsetY));
		if (Size.IsZero())
		{
			CanvasSlot->SetAutoSize(true);
		}
		else
		{
			CanvasSlot->SetSize(Size);
		}
		return CanvasSlot;
	};

	// --- Interaction prompt: [key chip] verb, low in the centre
	UHorizontalBox* PromptBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PromptPanel"));
	UBorder* KeyChip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptKeyChip"));
	KeyChip->SetBrush(FSlateRoundedBoxBrush(Parchment, 7.f, FLinearColor(0.35f, 0.28f, 0.16f, 1.f), 1.5f));
	KeyChip->SetPadding(FMargin(11.f, 2.f));
	PromptKeyText = MakeText(TEXT("PromptKeyText"), BoldFont, Ink);
	PromptKeyText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	KeyChip->SetContent(PromptKeyText);
	UHorizontalBoxSlot* ChipSlot = PromptBox->AddChildToHorizontalBox(KeyChip);
	ChipSlot->SetVerticalAlignment(VAlign_Center);
	PromptText = MakeText(TEXT("PromptText"), BoldFont, Cream);
	UHorizontalBoxSlot* VerbSlot = PromptBox->AddChildToHorizontalBox(PromptText);
	VerbSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
	VerbSlot->SetVerticalAlignment(VAlign_Center);
	PromptBox->SetVisibility(ESlateVisibility::Collapsed);
	PromptPanel = PromptBox;
	Place(PromptBox, 0.72f, 0.f);

	// --- Narration / companion line: rounded dark box at the bottom
	UBorder* MessageBox = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MessagePanel"));
	MessageBox->SetBrush(FSlateRoundedBoxBrush(Panel, 14.f));
	MessageBox->SetPadding(FMargin(28.f, 14.f));
	MessageText = MakeText(TEXT("MessageText"), BodyFont, Cream);
	MessageText->SetAutoWrapText(false);	// wrap at a fixed width: the canvas slot auto-sizes to the text
	MessageText->SetWrapTextAt(720.f);
	MessageText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	MessageBox->SetContent(MessageText);
	MessageBox->SetVisibility(ESlateVisibility::Collapsed);
	MessagePanel = MessageBox;
	Place(MessageBox, 0.86f, 0.f);

	// --- Boss: engraved name, quiet label, thin golden gauge
	BossNameText = MakeText(TEXT("BossNameText"), TitleFont, Gold);
	BossNameText->SetVisibility(ESlateVisibility::Collapsed);
	Place(BossNameText, 0.06f, 0.f);
	BossLabelText = MakeText(TEXT("BossLabelText"), ItalicFont, Muted);
	BossLabelText->SetVisibility(ESlateVisibility::Collapsed);
	Place(BossLabelText, 0.06f, 30.f);
	BossBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BossBar"));
	FProgressBarStyle BarStyle;
	BarStyle.SetBackgroundImage(FSlateRoundedBoxBrush(FLinearColor(0.02f, 0.02f, 0.03f, 0.6f), 4.f, FLinearColor(0.6f, 0.5f, 0.3f, 0.8f), 1.f));
	BarStyle.SetFillImage(FSlateRoundedBoxBrush(FLinearColor::White, 3.f));
	BarStyle.SetMarqueeImage(FSlateRoundedBoxBrush(FLinearColor::White, 3.f));
	BossBar->SetWidgetStyle(BarStyle);
	BossBar->SetFillColorAndOpacity(Gold);
	BossBar->SetVisibility(ESlateVisibility::Collapsed);
	Place(BossBar, 0.06f, 56.f, FVector2D(460.f, 8.f));

	// --- Hearts, top left
	HeartsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeartsBox"));
	UCanvasPanelSlot* HeartsSlot = Root->AddChildToCanvas(HeartsBox);
	HeartsSlot->SetAnchors(FAnchors(0.f, 0.f));
	HeartsSlot->SetAlignment(FVector2D(0.f, 0.f));
	HeartsSlot->SetPosition(FVector2D(42.f, 34.f));
	HeartsSlot->SetAutoSize(true);
	for (int32 Q = 0; Q <= 4; ++Q)
	{
		const FString Path = FString::Printf(TEXT("/Game/Project/UI/Textures/T_UI_Heart_%d.T_UI_Heart_%d"), Q, Q);
		HeartTextures.Add(LoadObject<UTexture2D>(nullptr, *Path));
	}

	// --- Stamina ring, beside the character
	StaminaRing = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("StaminaRing"));
	if (UMaterialInterface* RingMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Project/UI/Materials/M_UI_StaminaRing.M_UI_StaminaRing")))
	{
		StaminaMID = UMaterialInstanceDynamic::Create(RingMat, this);
		StaminaRing->SetBrushFromMaterial(StaminaMID);
	}
	StaminaRing->SetRenderOpacity(0.f);
	StaminaRing->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* RingSlot = Root->AddChildToCanvas(StaminaRing);
	RingSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	RingSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	RingSlot->SetPosition(FVector2D(120.f, -70.f));
	RingSlot->SetSize(FVector2D(72.f, 72.f));

	// --- Banner: big golden capitals
	BannerTitle = MakeText(TEXT("BannerTitle"), BannerFont, Gold);
	BannerTitle->SetVisibility(ESlateVisibility::Collapsed);
	Place(BannerTitle, 0.40f, 0.f);
	BannerSubtitle = MakeText(TEXT("BannerSubtitle"), ItalicLarge, Cream);
	BannerSubtitle->SetWrapTextAt(820.f);
	BannerSubtitle->SetVisibility(ESlateVisibility::Collapsed);
	Place(BannerSubtitle, 0.40f, 88.f);
}

void UEldoriaHUDWidget::SetBossStatus(bool bVisible, const FText& Name, const FText& Label, float Progress)
{
	const ESlateVisibility NewVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (BossNameText) { BossNameText->SetText(Name); BossNameText->SetVisibility(NewVisibility); }
	if (BossLabelText) { BossLabelText->SetText(Label); BossLabelText->SetVisibility(NewVisibility); }
	if (BossBar) { BossBar->SetPercent(FMath::Clamp(Progress, 0.f, 1.f)); BossBar->SetVisibility(NewVisibility); }
}

void UEldoriaHUDWidget::ShowBanner(const FText& Title, const FText& Subtitle, float Duration)
{
	if (BannerTitle) { BannerTitle->SetText(Title); BannerTitle->SetVisibility(ESlateVisibility::HitTestInvisible); }
	if (BannerSubtitle) { BannerSubtitle->SetText(Subtitle); BannerSubtitle->SetVisibility(ESlateVisibility::HitTestInvisible); }
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BannerTimer, this, &UEldoriaHUDWidget::HideBanner, FMath::Max(Duration, 1.f), false);
	}
}

void UEldoriaHUDWidget::HideBanner()
{
	if (BannerTitle) { BannerTitle->SetVisibility(ESlateVisibility::Collapsed); }
	if (BannerSubtitle) { BannerSubtitle->SetVisibility(ESlateVisibility::Collapsed); }
}

void UEldoriaHUDWidget::SetHealth(int32 Quarters, int32 MaxQuarters)
{
	if (!HeartsBox)
	{
		return;
	}
	const int32 Hearts = FMath::Max(1, (MaxQuarters + 3) / 4);
	while (HeartImages.Num() < Hearts)
	{
		UImage* Heart = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Heart->SetDesiredSizeOverride(FVector2D(40.f, 40.f));
		UHorizontalBoxSlot* HeartSlot = HeartsBox->AddChildToHorizontalBox(Heart);
		HeartSlot->SetPadding(FMargin(0.f, 0.f, 5.f, 0.f));
		HeartImages.Add(Heart);
	}
	for (int32 Index = 0; Index < HeartImages.Num(); ++Index)
	{
		const int32 Fill = FMath::Clamp(Quarters - Index * 4, 0, 4);
		if (HeartTextures.IsValidIndex(Fill) && HeartTextures[Fill])
		{
			HeartImages[Index]->SetBrushFromTexture(HeartTextures[Fill]);
			HeartImages[Index]->SetDesiredSizeOverride(FVector2D(40.f, 40.f));
		}
		HeartImages[Index]->SetVisibility(Index < Hearts ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (LastQuarters >= 0 && Quarters < LastQuarters)
	{
		DamageFlash = 1.f;
	}
	LastQuarters = Quarters;
}

void UEldoriaHUDWidget::SetStamina(float Stamina, bool bExhausted)
{
	StaminaValue = Stamina;
	StaminaFullFor = Stamina >= 1.f ? StaminaFullFor : 0.f;
	if (StaminaMID)
	{
		StaminaMID->SetScalarParameterValue(TEXT("Percent"), Stamina);
		StaminaMID->SetVectorParameterValue(TEXT("Fill"), bExhausted ? FLinearColor(0.92f, 0.22f, 0.16f) : FLinearColor(0.42f, 0.86f, 0.36f));
	}
}

void UEldoriaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// the ring lingers a moment once full, then fades away
	StaminaFullFor = StaminaValue >= 1.f ? StaminaFullFor + InDeltaTime : 0.f;
	const float Target = StaminaFullFor < 0.8f ? 1.f : 0.f;
	StaminaOpacity = FMath::FInterpTo(StaminaOpacity, Target, InDeltaTime, Target > 0.f ? 12.f : 3.f);
	if (StaminaRing)
	{
		StaminaRing->SetRenderOpacity(StaminaOpacity);
	}
	// hearts pulse briefly when hurt
	if (HeartsBox)
	{
		DamageFlash = FMath::Max(0.f, DamageFlash - InDeltaTime * 2.5f);
		const float Scale = 1.f + 0.18f * DamageFlash;
		HeartsBox->SetRenderScale(FVector2D(Scale, Scale));
	}
}

void UEldoriaHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UEldoriaMessageSubsystem* Messages = GetWorld() ? GetWorld()->GetSubsystem<UEldoriaMessageSubsystem>() : nullptr)
	{
		Messages->OnMessageShown.AddUniqueDynamic(this, &UEldoriaHUDWidget::HandleMessageShown);
	}
}

void UEldoriaHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MessageTimer);
		if (UEldoriaMessageSubsystem* Messages = World->GetSubsystem<UEldoriaMessageSubsystem>())
		{
			Messages->OnMessageShown.RemoveDynamic(this, &UEldoriaHUDWidget::HandleMessageShown);
		}
	}
	Super::NativeDestruct();
}

void UEldoriaHUDWidget::SetInteractionPrompt(const FText& Prompt)
{
	if (!PromptText)
	{
		return;
	}

	UWidget* Shown = PromptPanel ? PromptPanel.Get() : static_cast<UWidget*>(PromptText.Get());
	if (Prompt.IsEmpty())
	{
		Shown->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (PromptKeyText)
	{
		PromptKeyText->SetText(InteractKeyHint);
		PromptText->SetText(Prompt);
	}
	else
	{
		PromptText->SetText(FText::Format(NSLOCTEXT("EldoriaHUD", "PromptFormat", "[{0}]  {1}"), InteractKeyHint, Prompt));
	}
	PromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
	Shown->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEldoriaHUDWidget::ShowMessage(const FText& Message, float Duration)
{
	if (!MessageText)
	{
		return;
	}

	MessageText->SetText(Message);
	MessageText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (MessagePanel)
	{
		MessagePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MessageTimer, this, &UEldoriaHUDWidget::HideMessage, FMath::Max(Duration, 0.5f), false);
	}
}

void UEldoriaHUDWidget::HideMessage()
{
	if (MessagePanel)
	{
		MessagePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	else if (MessageText)
	{
		MessageText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEldoriaHUDWidget::HandleMessageShown(const FText& Message, float Duration)
{
	ShowMessage(Message, Duration);
}
