#include "OBHUDWidget.h"

#include "Kismet/GameplayStatics.h"
#include "OBCharacter.h"

void UOBHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromGameState();
	OnHudInitialized();
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

	CurrentSurvivalTime = OneBulletState->SurvivalTime;
	CurrentLastRunTime = OneBulletState->LastRunTime;
	CurrentBestKillCount = OneBulletState->BestKillCount;
	CurrentDeathReason = OneBulletState->DeathReason;

	bool bChanged = false;
	if (CurrentBulletState != OneBulletState->BulletState)
	{
		const bool bBulletWasLost = CurrentBulletState == EBulletState::Lost;
		CurrentBulletState = OneBulletState->BulletState;
		OnBulletStateChanged(CurrentBulletState);
		if (bBulletWasLost && CurrentBulletState == EBulletState::Ready)
		{
			OnBulletRecovered();
		}
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
			OnImmortalModeChanged(bCurrentImmortalMode);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		OnHudStateRefreshed(CurrentBulletState, CurrentKillCount, bCurrentGameOver);
	}
}
