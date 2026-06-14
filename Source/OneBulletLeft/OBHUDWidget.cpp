#include "OBHUDWidget.h"

#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "OBCharacter.h"

void UOBHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromGameState();
	OnHudInitialized();
	ApplyHudTextColors();

	if (WaveTxt)
	{
		WaveTxt->SetRenderOpacity(0.0f);
		WaveTxt->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (EnemiesLeftTxt)
	{
		EnemiesLeftTxt->SetRenderOpacity(0.0f);
		EnemiesLeftTxt->SetVisibility(ESlateVisibility::Collapsed);
	}

	TryBindWaveManager();
	RefreshFromWaveManager();
}

void UOBHUDWidget::NativeDestruct()
{
	UnbindWaveManager();
	Super::NativeDestruct();
}

void UOBHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshFromGameState();
	RefreshFromWaveManager();
	UpdateCountdown(InDeltaTime);
	UpdateWaveTextAnimations(InDeltaTime);
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

	ApplyHudTextColors();
}

void UOBHUDWidget::ApplyHudTextColors()
{
	if (BulletStatusText)
	{
		BulletStatusText->SetColorAndOpacity(
			CurrentBulletState == EBulletState::Ready
				? BulletReadyTextColor
				: BulletLostTextColor);
	}

	if (KillCountText)
	{
		KillCountText->SetColorAndOpacity(KillCountTextColor);
	}
}

void UOBHUDWidget::RefreshFromWaveManager()
{
	if (!IsValid(WaveManager))
	{
		TryBindWaveManager();
	}
	if (!IsValid(WaveManager))
	{
		return;
	}

	if (CurrentWaveState != WaveManager->WaveState)
	{
		HandleWaveStateChanged(WaveManager->WaveState, CurrentWaveState);
	}

	CurrentWaveNumber = WaveManager->CurrentWaveNumber;
	const int32 NewEnemiesLeft = FMath::Max(
		WaveManager->EnemiesRemainingToSpawn + WaveManager->LivingEnemyCount,
		0);

	if (CurrentWaveState == EOBWaveState::Active && CurrentEnemiesLeft != NewEnemiesLeft)
	{
		CurrentEnemiesLeft = NewEnemiesLeft;
		SetEnemiesMessage(
			FText::Format(
				NSLOCTEXT("OneBulletWaveUI", "EnemiesLeft", "Enemies Left: {0}"),
				FText::AsNumber(CurrentEnemiesLeft)),
			false);
	}
}

void UOBHUDWidget::TryBindWaveManager()
{
	if (IsValid(WaveManager) || !GetWorld())
	{
		return;
	}

	WaveManager = Cast<AOBWaveManager>(
		UGameplayStatics::GetActorOfClass(this, AOBWaveManager::StaticClass()));
	if (!IsValid(WaveManager))
	{
		return;
	}

	WaveManager->OnWaveStateChanged.AddUniqueDynamic(this, &UOBHUDWidget::HandleWaveStateChanged);
	WaveManager->OnWaveStarted.AddUniqueDynamic(this, &UOBHUDWidget::HandleWaveStarted);
	WaveManager->OnWaveCompleted.AddUniqueDynamic(this, &UOBHUDWidget::HandleWaveCompleted);

	CurrentWaveState = WaveManager->WaveState;
	CurrentWaveNumber = WaveManager->CurrentWaveNumber;

	switch (CurrentWaveState)
	{
	case EOBWaveState::Active:
		HandleWaveStarted(
			CurrentWaveNumber,
			WaveManager->EnemiesRemainingToSpawn + WaveManager->LivingEnemyCount,
			WaveManager->CurrentDifficultyMultiplier);
		break;
	case EOBWaveState::Completed:
		HandleWaveCompleted(CurrentWaveNumber);
		break;
	case EOBWaveState::Intermission:
		CurrentNextWaveSeconds = FMath::Max(
			FMath::CeilToInt(WaveManager->GetIntermissionTimeRemaining()),
			1);
		SetWaveMessage(
			FText::Format(
				NSLOCTEXT("OneBulletWaveUI", "NextWaveCountdown", "Next Wave in {0}..."),
				FText::AsNumber(CurrentNextWaveSeconds)),
			true);
		HideEnemiesMessage();
		break;
	default:
		HideWaveMessage();
		HideEnemiesMessage();
		break;
	}
}

void UOBHUDWidget::UnbindWaveManager()
{
	if (IsValid(WaveManager))
	{
		WaveManager->OnWaveStateChanged.RemoveDynamic(this, &UOBHUDWidget::HandleWaveStateChanged);
		WaveManager->OnWaveStarted.RemoveDynamic(this, &UOBHUDWidget::HandleWaveStarted);
		WaveManager->OnWaveCompleted.RemoveDynamic(this, &UOBHUDWidget::HandleWaveCompleted);
	}
	WaveManager = nullptr;
}

void UOBHUDWidget::HandleWaveStateChanged(EOBWaveState NewState, EOBWaveState PreviousState)
{
	CurrentWaveState = NewState;

	switch (NewState)
	{
	case EOBWaveState::Waiting:
		CurrentNextWaveSeconds = 0;
		ClearedMessageTimeRemaining = 0.0f;
		HideWaveMessage();
		HideEnemiesMessage();
		break;
	case EOBWaveState::Active:
		ClearedMessageTimeRemaining = 0.0f;
		break;
	case EOBWaveState::Completed:
		HandleWaveCompleted(WaveManager ? WaveManager->CurrentWaveNumber : CurrentWaveNumber);
		break;
	case EOBWaveState::Intermission:
		ClearedMessageTimeRemaining = FMath::Max(WaveClearedDisplayDuration, 0.0f);
		HideEnemiesMessage();
		break;
	default:
		break;
	}
}

void UOBHUDWidget::HandleWaveStarted(int32 WaveNumber, int32 EnemyCount, float DifficultyMultiplier)
{
	CurrentWaveState = EOBWaveState::Active;
	CurrentWaveNumber = WaveNumber;
	CurrentEnemiesLeft = FMath::Max(EnemyCount, 0);
	CurrentNextWaveSeconds = 0;
	ClearedMessageTimeRemaining = 0.0f;

	SetWaveMessage(
		FText::Format(
			NSLOCTEXT("OneBulletWaveUI", "WaveNumber", "Wave {0}"),
			FText::AsNumber(CurrentWaveNumber)),
		true);
	SetEnemiesMessage(
		FText::Format(
			NSLOCTEXT("OneBulletWaveUI", "EnemiesLeft", "Enemies Left: {0}"),
			FText::AsNumber(CurrentEnemiesLeft)),
		true);
}

void UOBHUDWidget::HandleWaveCompleted(int32 WaveNumber)
{
	CurrentWaveNumber = WaveNumber;
	CurrentEnemiesLeft = 0;
	ClearedMessageTimeRemaining = FMath::Max(WaveClearedDisplayDuration, 0.0f);
	SetWaveMessage(NSLOCTEXT("OneBulletWaveUI", "WaveCleared", "Wave Cleared"), true);
	HideEnemiesMessage();
}

void UOBHUDWidget::SetWaveMessage(const FText& Message, bool bAnimate)
{
	if (!WaveTxt)
	{
		return;
	}

	const bool bTextChanged = !WaveTxt->GetText().EqualTo(Message);
	if (bAnimate && bTextChanged && WaveTextOpacity > 0.01f)
	{
		PendingWaveMessage = Message;
		bHasPendingWaveMessage = true;
		WaveTextTargetOpacity = 0.0f;
		return;
	}

	WaveTxt->SetText(Message);
	WaveTxt->SetVisibility(ESlateVisibility::HitTestInvisible);
	WaveTextTargetOpacity = 1.0f;

	if (bAnimate && bTextChanged)
	{
		WaveTextOpacity = 0.0f;
		WaveTxt->SetRenderOpacity(0.0f);
	}
}

void UOBHUDWidget::SetEnemiesMessage(const FText& Message, bool bAnimate)
{
	if (!EnemiesLeftTxt)
	{
		return;
	}

	const bool bTextChanged = !EnemiesLeftTxt->GetText().EqualTo(Message);
	EnemiesLeftTxt->SetText(Message);
	EnemiesLeftTxt->SetVisibility(ESlateVisibility::HitTestInvisible);
	EnemiesTextTargetOpacity = 1.0f;

	if (bAnimate && bTextChanged)
	{
		EnemiesTextOpacity = 0.0f;
		EnemiesLeftTxt->SetRenderOpacity(0.0f);
	}
}

void UOBHUDWidget::HideWaveMessage()
{
	bHasPendingWaveMessage = false;
	PendingWaveMessage = FText::GetEmpty();
	WaveTextTargetOpacity = 0.0f;
}

void UOBHUDWidget::HideEnemiesMessage()
{
	EnemiesTextTargetOpacity = 0.0f;
}

void UOBHUDWidget::UpdateWaveTextAnimations(float DeltaTime)
{
	const float Speed = FMath::Max(WaveTextAnimationSpeed, 0.1f);
	const float HiddenScale = FMath::Clamp(WaveTextHiddenScale, 0.5f, 1.0f);

	if (WaveTxt)
	{
		WaveTextOpacity = FMath::FInterpTo(WaveTextOpacity, WaveTextTargetOpacity, DeltaTime, Speed);
		WaveTxt->SetRenderOpacity(WaveTextOpacity);
		const float Scale = FMath::Lerp(HiddenScale, 1.0f, WaveTextOpacity);
		WaveTxt->SetRenderScale(FVector2D(Scale));

		if (WaveTextTargetOpacity <= 0.0f && WaveTextOpacity <= 0.01f)
		{
			if (bHasPendingWaveMessage)
			{
				WaveTxt->SetText(PendingWaveMessage);
				WaveTxt->SetVisibility(ESlateVisibility::HitTestInvisible);
				PendingWaveMessage = FText::GetEmpty();
				bHasPendingWaveMessage = false;
				WaveTextTargetOpacity = 1.0f;
			}
			else
			{
				WaveTxt->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	if (EnemiesLeftTxt)
	{
		EnemiesTextOpacity = FMath::FInterpTo(EnemiesTextOpacity, EnemiesTextTargetOpacity, DeltaTime, Speed);
		EnemiesLeftTxt->SetRenderOpacity(EnemiesTextOpacity);
		const float Scale = FMath::Lerp(HiddenScale, 1.0f, EnemiesTextOpacity);
		EnemiesLeftTxt->SetRenderScale(FVector2D(Scale));

		if (EnemiesTextTargetOpacity <= 0.0f && EnemiesTextOpacity <= 0.01f)
		{
			EnemiesLeftTxt->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UOBHUDWidget::UpdateCountdown(float DeltaTime)
{
	if (CurrentWaveState != EOBWaveState::Intermission || !IsValid(WaveManager))
	{
		return;
	}

	if (ClearedMessageTimeRemaining > 0.0f)
	{
		ClearedMessageTimeRemaining = FMath::Max(ClearedMessageTimeRemaining - DeltaTime, 0.0f);
		if (ClearedMessageTimeRemaining > 0.0f)
		{
			return;
		}
	}

	const int32 NewSeconds = FMath::Max(
		FMath::CeilToInt(WaveManager->GetIntermissionTimeRemaining()),
		1);
	if (CurrentNextWaveSeconds == NewSeconds)
	{
		return;
	}

	CurrentNextWaveSeconds = NewSeconds;
	SetWaveMessage(
		FText::Format(
			NSLOCTEXT("OneBulletWaveUI", "NextWaveCountdown", "Next Wave in {0}..."),
			FText::AsNumber(CurrentNextWaveSeconds)),
		true);
}
