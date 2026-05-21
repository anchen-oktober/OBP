#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OBGameState.h"
#include "OBHUDWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;

UCLASS(Blueprintable)
class ONEBULLETLEFT_API UOBHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	EBulletState CurrentBulletState = EBulletState::Ready;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	int32 CurrentKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	bool bCurrentGameOver = false;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	float CurrentSurvivalTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	float CurrentLastRunTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	int32 CurrentBestKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	FText CurrentDeathReason = FText::GetEmpty();

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	bool bCurrentDodgeReady = true;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	float CurrentDodgeCooldownNormalized = 0.0f;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> BulletStatusText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> KillCountText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> GameOverText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> RestartText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> DeathStatsText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> DodgeStatusText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UProgressBar> DodgeCooldownBar;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UBorder> DeathFade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText BulletReadyText = FText::FromString(TEXT("Bullet: Ready"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText BulletLostText = FText::FromString(TEXT("Bullet: Lost"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText KillCountFormat = FText::FromString(TEXT("Kills: {0}"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText DeathStatsFormat = FText::FromString(TEXT("{0}\nKills: {1}\nTime: {2}s\nBest: {3}"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText DodgeReadyText = FText::FromString(TEXT("Dodge: Ready"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText DodgeCooldownFormat = FText::FromString(TEXT("Dodge: {0}s"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Colors")
	FLinearColor BulletReadyColor = FLinearColor(0.35f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Colors")
	FLinearColor BulletLostColor = FLinearColor(1.0f, 0.45f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Death Fade", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DeathFadeMaxAlpha = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Death Fade")
	FLinearColor DeathFadeColor = FLinearColor::Black;

	UFUNCTION(BlueprintPure, Category="One Bullet|HUD")
	bool IsBulletReady() const { return CurrentBulletState == EBulletState::Ready; }

	UFUNCTION(BlueprintPure, Category="One Bullet|HUD")
	int32 GetKillCount() const { return CurrentKillCount; }

	UFUNCTION(BlueprintPure, Category="One Bullet|HUD")
	bool IsGameOver() const { return bCurrentGameOver; }

	UFUNCTION(BlueprintPure, Category="One Bullet|HUD")
	float GetSurvivalTime() const { return CurrentSurvivalTime; }

	UFUNCTION(BlueprintPure, Category="One Bullet|HUD")
	int32 GetBestKillCount() const { return CurrentBestKillCount; }

	UFUNCTION(BlueprintPure, Category="One Bullet|HUD")
	bool IsDodgeReady() const { return bCurrentDodgeReady; }

	UFUNCTION(BlueprintCallable, Category="One Bullet|HUD")
	void RefreshFromGameState();

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnBulletStateChanged(EBulletState NewBulletState);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnKillCountChanged(int32 NewKillCount);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnGameOverChanged(bool bNewGameOver);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnDodgeCooldownChanged(bool bNewDodgeReady, float NewCooldownNormalized);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnHudStateRefreshed(EBulletState NewBulletState, int32 NewKillCount, bool bNewGameOver);

private:
	void ApplyHudState();
};
