#include "OBHUDWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UOBHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromGameState();
	ApplyHudState();
}

void UOBHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshFromGameState();
}

void UOBHUDWidget::RefreshFromGameState()
{
	const AOBGameState* OneBulletState = GetWorld() ? GetWorld()->GetGameState<AOBGameState>() : nullptr;
	if (!OneBulletState)
	{
		return;
	}

	bool bChanged = false;
	if (CurrentBulletState != OneBulletState->BulletState)
	{
		CurrentBulletState = OneBulletState->BulletState;
		OnBulletStateChanged(CurrentBulletState);
		bChanged = true;
	}

	if (CurrentKillCount != OneBulletState->KillCount)
	{
		CurrentKillCount = OneBulletState->KillCount;
		OnKillCountChanged(CurrentKillCount);
		bChanged = true;
	}

	if (bCurrentGameOver != OneBulletState->bGameOver)
	{
		bCurrentGameOver = OneBulletState->bGameOver;
		OnGameOverChanged(bCurrentGameOver);
		bChanged = true;
	}

	if (bChanged)
	{
		ApplyHudState();
		OnHudStateRefreshed(CurrentBulletState, CurrentKillCount, bCurrentGameOver);
	}
}

void UOBHUDWidget::ApplyHudState()
{
	const bool bBulletReady = CurrentBulletState == EBulletState::Ready;

	if (BulletStatusText)
	{
		BulletStatusText->SetText(bBulletReady ? BulletReadyText : BulletLostText);
		BulletStatusText->SetColorAndOpacity(bBulletReady ? BulletReadyColor : BulletLostColor);
	}

	if (KillCountText)
	{
		KillCountText->SetText(FText::Format(KillCountFormat, FText::AsNumber(CurrentKillCount)));
	}

	if (GameOverText)
	{
		GameOverText->SetVisibility(bCurrentGameOver ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (RestartText)
	{
		RestartText->SetVisibility(bCurrentGameOver ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (DeathFade)
	{
		FLinearColor FadeColor = DeathFadeColor;
		FadeColor.A = bCurrentGameOver ? DeathFadeMaxAlpha : 0.0f;
		DeathFade->SetBrushColor(FadeColor);
		DeathFade->SetVisibility(bCurrentGameOver ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
