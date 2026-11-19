// Project Eldoria

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EldoriaHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;

/**
 * Minimal gameplay HUD: contextual interaction prompt + transient message line.
 *
 * Works on its own (builds a default layout in C++), but a Widget Blueprint can
 * subclass it and provide its own layout: it only needs TextBlocks named
 * "PromptText" and "MessageText" (both optional).
 */
UCLASS()
class UEldoriaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Shows the prompt for the focused interactable. Empty text hides it. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetInteractionPrompt(const FText& Prompt);

	/** Shows a message for Duration seconds. */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowMessage(const FText& Message, float Duration);

	/** Boss status: name + how much of the control remains (Progress 0..1 = liberation progress). */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void SetBossStatus(bool bVisible, const FText& Name, const FText& Label, float Progress);

	/** Big centred title (ability learnt, chapter...). */
	UFUNCTION(BlueprintCallable, Category="HUD")
	void ShowBanner(const FText& Title, const FText& Subtitle, float Duration);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Key hint displayed before the prompt verb. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HUD")
	FText InteractKeyHint = NSLOCTEXT("EldoriaHUD", "InteractKey", "[E]");

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BossLabelText;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> BossBar;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BannerTitle;

	UPROPERTY(BlueprintReadOnly, Category="HUD", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BannerSubtitle;

private:

	void BuildDefaultLayout();
	void HideMessage();
	void HideBanner();

	FTimerHandle BannerTimer;

	UFUNCTION()
	void HandleMessageShown(const FText& Message, float Duration);

	FTimerHandle MessageTimer;
};
