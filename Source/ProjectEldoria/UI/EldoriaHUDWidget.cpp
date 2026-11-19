// Project Eldoria

#include "UI/EldoriaHUDWidget.h"
#include "UI/EldoriaMessageSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "TimerManager.h"

TSharedRef<SWidget> UEldoriaHUDWidget::RebuildWidget()
{
	// Pure C++ instance (no designer tree): build a default layout
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultLayout();
	}
	return Super::RebuildWidget();
}

void UEldoriaHUDWidget::BuildDefaultLayout()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	auto MakeLine = [this, Root](FName Name, float AnchorY, float OffsetY, float FontSize) -> UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetJustification(ETextJustify::Center);
		Text->SetFontSize(FontSize);
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.97f, 0.88f)));
		Text->SetVisibility(ESlateVisibility::Collapsed);

		UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Text);
		CanvasSlot->SetAnchors(FAnchors(0.5f, AnchorY));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetPosition(FVector2D(0.f, OffsetY));
		return Text;
	};

	PromptText = MakeLine(TEXT("PromptText"), 0.78f, 0.f, 20.f);
	MessageText = MakeLine(TEXT("MessageText"), 0.3f, 0.f, 22.f);

	// Boss status, top of the screen: name, label, bar
	BossNameText = MakeLine(TEXT("BossNameText"), 0.05f, 0.f, 26.f);
	BossLabelText = MakeLine(TEXT("BossLabelText"), 0.05f, 30.f, 14.f);
	BossBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BossBar"));
	BossBar->SetFillColorAndOpacity(FLinearColor(0.35f, 0.85f, 0.7f, 1.f));
	BossBar->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* BarSlot = Root->AddChildToCanvas(BossBar);
	BarSlot->SetAnchors(FAnchors(0.5f, 0.05f));
	BarSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	BarSlot->SetSize(FVector2D(520.f, 12.f));
	BarSlot->SetPosition(FVector2D(0.f, 56.f));

	// Banner, centre
	BannerTitle = MakeLine(TEXT("BannerTitle"), 0.42f, 0.f, 54.f);
	BannerSubtitle = MakeLine(TEXT("BannerSubtitle"), 0.42f, 58.f, 18.f);
	BannerTitle->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.88f, 0.55f)));
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

	if (Prompt.IsEmpty())
	{
		PromptText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	PromptText->SetText(FText::Format(NSLOCTEXT("EldoriaHUD", "PromptFormat", "{0}  {1}"), InteractKeyHint, Prompt));
	PromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEldoriaHUDWidget::ShowMessage(const FText& Message, float Duration)
{
	if (!MessageText)
	{
		return;
	}

	MessageText->SetText(Message);
	MessageText->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MessageTimer, this, &UEldoriaHUDWidget::HideMessage, FMath::Max(Duration, 0.5f), false);
	}
}

void UEldoriaHUDWidget::HideMessage()
{
	if (MessageText)
	{
		MessageText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEldoriaHUDWidget::HandleMessageShown(const FText& Message, float Duration)
{
	ShowMessage(Message, Duration);
}
