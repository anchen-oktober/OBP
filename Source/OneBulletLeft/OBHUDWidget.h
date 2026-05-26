#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OBGameState.h"
#include "OBHUDWidget.generated.h"

UCLASS(Blueprintable, PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API UOBHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
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

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD")
	void RefreshFromGameState();

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

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|HUD")
	void OnHudStateRefreshed(EBulletState NewBulletState, int32 NewKillCount, bool bNewGameOver);
};
