#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OBGameState.h"
#include "OBHUDWidget.generated.h"

class UBorder;
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

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> BulletStatusText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> KillCountText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> GameOverText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UTextBlock> RestartText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="One Bullet|HUD|Widgets")
	TObjectPtr<UBorder> DeathFade;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText BulletReadyText = FText::FromString(TEXT("Bullet: Ready"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText BulletLostText = FText::FromString(TEXT("Bullet: Lost"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|HUD|Text")
	FText KillCountFormat = FText::FromString(TEXT("Kills: {0}"));

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

	UFUNCTION(BlueprintCallable, Category="One Bullet|HUD")
	void RefreshFromGameState();

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnBulletStateChanged(EBulletState NewBulletState);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnKillCountChanged(int32 NewKillCount);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnGameOverChanged(bool bNewGameOver);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|HUD")
	void OnHudStateRefreshed(EBulletState NewBulletState, int32 NewKillCount, bool bNewGameOver);

private:
	void ApplyHudState();
};
