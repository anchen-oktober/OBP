#include "OBHUDWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "OBCharacter.h"

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

	CurrentSurvivalTime = OneBulletState->SurvivalTime;
	CurrentLastRunTime = OneBulletState->LastRunTime;
	CurrentBestKillCount = OneBulletState->BestKillCount;
	CurrentDeathReason = OneBulletState->DeathReason;

	if (const AOBCharacter* Player = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		const bool bNewDodgeReady = Player->IsDodgeReady();
		const float NewDodgeCooldownNormalized = Player->GetDodgeCooldownNormalized();
		if (bCurrentDodgeReady != bNewDodgeReady || !FMath::IsNearlyEqual(CurrentDodgeCooldownNormalized, NewDodgeCooldownNormalized, 0.01f))
		{
			bCurrentDodgeReady = bNewDodgeReady;
			CurrentDodgeCooldownNormalized = NewDodgeCooldownNormalized;
			OnDodgeCooldownChanged(bCurrentDodgeReady, CurrentDodgeCooldownNormalized);
			bChanged = true;
		}

		if (bCurrentImmortalMode != Player->IsImmortalMode())
		{
			bCurrentImmortalMode = Player->IsImmortalMode();
			bChanged = true;
		}
	}

	if (bChanged || bCurrentGameOver || CurrentBulletState == EBulletState::Lost)
	{
		ApplyHudState();
		if (bChanged)
		{
			OnHudStateRefreshed(CurrentBulletState, CurrentKillCount, bCurrentGameOver);
		}
	}
}

void UOBHUDWidget::ApplyHudState()
{
	const bool bBulletReady = CurrentBulletState == EBulletState::Ready;

	if (BulletStatusText)
	{
		BulletStatusText->SetText(bBulletReady ? BulletReadyText : BulletLostText);
		FLinearColor BulletColor = bBulletReady ? BulletReadyColor : BulletLostColor;
		if (!bBulletReady && GetWorld())
		{
			BulletColor.A = 0.72f + 0.28f * (0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 8.0f));
		}
		BulletStatusText->SetColorAndOpacity(BulletColor);
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

	if (DeathStatsText)
	{
		const int32 RoundedTime = FMath::RoundToInt(CurrentLastRunTime);
		DeathStatsText->SetText(FText::Format(DeathStatsFormat, CurrentDeathReason, FText::AsNumber(CurrentKillCount), FText::AsNumber(RoundedTime), FText::AsNumber(CurrentBestKillCount)));
		DeathStatsText->SetVisibility(bCurrentGameOver ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (DodgeStatusText)
	{
		if (bCurrentDodgeReady)
		{
			DodgeStatusText->SetText(DodgeReadyText);
		}
		else
		{
			const int32 RemainingTenths = FMath::CeilToInt(CurrentDodgeCooldownNormalized * 10.0f);
			DodgeStatusText->SetText(FText::Format(DodgeCooldownFormat, FText::AsNumber(RemainingTenths / 10.0f)));
		}
	}

	if (DodgeCooldownBar)
	{
		DodgeCooldownBar->SetPercent(bCurrentDodgeReady ? 1.0f : 1.0f - CurrentDodgeCooldownNormalized);
	}

	if (LostWarningText)
	{
		LostWarningText->SetText(LostWarningMessage);
		LostWarningText->SetVisibility(!bBulletReady && !bCurrentGameOver ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ImmortalModeTxt)
	{
		ImmortalModeTxt->SetVisibility(bCurrentImmortalMode ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ChangeViewBtn)
	{
		ChangeViewBtn->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (DeathFade)
	{
		FLinearColor FadeColor = DeathFadeColor;
		FadeColor.A = bCurrentGameOver ? DeathFadeMaxAlpha : 0.0f;
		DeathFade->SetBrushColor(FadeColor);
		DeathFade->SetVisibility(bCurrentGameOver ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
