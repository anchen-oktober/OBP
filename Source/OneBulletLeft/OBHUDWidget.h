#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OBGameState.h"
#include "OBWaveManager.h"
#include "OBHUDWidget.generated.h"

class UTextBlock;
class USlider;
class UWidget;

UCLASS(Blueprintable, PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API UOBHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Gameplay state exposed for WBP_OBHUD; widget layout and visuals stay in Blueprint.
	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	EBulletState CurrentBulletState = EBulletState::Ready;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	int32 CurrentKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	bool bCurrentGameOver = false;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	float CurrentSurvivalTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	float CurrentLastRunTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	int32 CurrentBestKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	FText CurrentDeathReason = FText::GetEmpty();

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	bool bCurrentDodgeReady = true;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	float CurrentDodgeCooldownNormalized = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	bool bCurrentImmortalMode = false;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="OneBulletSettings|HUD|Text")
	TObjectPtr<UTextBlock> BulletStatusText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="OneBulletSettings|HUD|Text")
	TObjectPtr<UTextBlock> KillCountText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="OneBulletSettings|HUD|Settings")
	TObjectPtr<USlider> MouseSensitivitySlider;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="OneBulletSettings|HUD|Settings")
	TObjectPtr<UTextBlock> MouseSensitivityValueText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Text|Color")
	FLinearColor BulletReadyTextColor = FLinearColor(0.35f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Text|Color")
	FLinearColor BulletLostTextColor = FLinearColor(1.0f, 0.45f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Text|Color")
	FLinearColor KillCountTextColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="OneBulletSettings|HUD|Waves")
	TObjectPtr<UTextBlock> WaveTxt;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="OneBulletSettings|HUD|Waves")
	TObjectPtr<UTextBlock> EnemiesLeftTxt;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="OneBulletSettings|HUD|Text")
	TObjectPtr<UWidget> ImmortalModeMsg;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD|Waves")
	EOBWaveState CurrentWaveState = EOBWaveState::Waiting;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD|Waves")
	int32 CurrentWaveNumber = 0;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD|Waves")
	int32 CurrentEnemiesLeft = 0;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD|Waves")
	int32 CurrentNextWaveSeconds = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Waves|Animation", meta=(ClampMin="0.1"))
	float WaveTextAnimationSpeed = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Waves|Animation", meta=(ClampMin="0.5", ClampMax="1.0"))
	float WaveTextHiddenScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Waves|Animation", meta=(ClampMin="0.0"))
	float WaveClearedDisplayDuration = 0.85f;

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	bool IsBulletReady() const { return CurrentBulletState == EBulletState::Ready; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	int32 GetKillCount() const { return CurrentKillCount; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	bool IsGameOver() const { return bCurrentGameOver; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	float GetSurvivalTime() const { return CurrentSurvivalTime; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	float GetLastRunTime() const { return CurrentLastRunTime; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	int32 GetBestKillCount() const { return CurrentBestKillCount; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	FText GetDeathReason() const { return CurrentDeathReason; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	bool IsDodgeReady() const { return bCurrentDodgeReady; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	float GetDodgeCooldownNormalized() const { return CurrentDodgeCooldownNormalized; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD")
	bool IsImmortalMode() const { return bCurrentImmortalMode; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD|Settings")
	float GetMouseSensitivity() const;

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD|Settings")
	float GetMouseSensitivityNormalized() const;

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|HUD|Settings")
	FText GetMouseSensitivityText() const;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD")
	void RefreshFromGameState();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD|Settings")
	void SetMouseSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD|Settings")
	void SetMouseSensitivityNormalized(float NormalizedValue);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD|Settings")
	void ResetMouseSensitivity();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD|Waves")
	void RefreshFromWaveManager();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD")
	void ShowImmortalModeMsg();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD")
	void HideImmortalModeMsg();

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnHudInitialized();

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnBulletStateChanged(EBulletState NewBulletState);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnBulletRecovered();

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnKillCountChanged(int32 NewKillCount);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnGameOverChanged(bool bNewGameOver);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnDodgeCooldownChanged(bool bNewDodgeReady, float NewCooldownNormalized);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnImmortalModeChanged(bool bNewImmortalMode);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD|Settings")
	void OnMouseSensitivityChanged(float NewSensitivity, float NewNormalizedValue);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnHudStateRefreshed(EBulletState NewBulletState, int32 NewKillCount, bool bNewGameOver);

private:
	UPROPERTY(Transient)
	TObjectPtr<AOBWaveManager> WaveManager;

	float WaveTextOpacity = 0.0f;
	float EnemiesTextOpacity = 0.0f;
	float WaveTextTargetOpacity = 0.0f;
	float EnemiesTextTargetOpacity = 0.0f;
	float ClearedMessageTimeRemaining = 0.0f;
	FText PendingWaveMessage = FText::GetEmpty();
	bool bHasPendingWaveMessage = false;
	bool bUpdatingMouseSensitivitySlider = false;
	FTimerHandle ImmortalModeMsgTimerHandle;

	void TryBindWaveManager();
	void UnbindWaveManager();
	void SetWaveMessage(const FText& Message, bool bAnimate);
	void SetEnemiesMessage(const FText& Message, bool bAnimate);
	void HideWaveMessage();
	void HideEnemiesMessage();
	void UpdateWaveTextAnimations(float DeltaTime);
	void UpdateCountdown(float DeltaTime);
	void ApplyHudTextColors();
	void RefreshMouseSensitivityControls();

	UFUNCTION()
	void HandleMouseSensitivitySliderChanged(float NewValue);

	UFUNCTION()
	void HandleWaveStateChanged(EOBWaveState NewState, EOBWaveState PreviousState);

	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber, int32 EnemyCount, float DifficultyMultiplier);

	UFUNCTION()
	void HandleWaveCompleted(int32 WaveNumber);
};
