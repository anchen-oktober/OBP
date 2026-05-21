#include "OBGameState.h"

namespace
{
int32 GOneBulletBestKillCount = 0;
}

AOBGameState::AOBGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	BulletState = EBulletState::Ready;
	KillCount = 0;
	bGameOver = false;
	SurvivalTime = 0.0f;
	LastRunTime = 0.0f;
	BestKillCount = GOneBulletBestKillCount;
	DeathReason = FText::GetEmpty();
}

void AOBGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bGameOver)
	{
		SurvivalTime += DeltaSeconds;
	}
}

void AOBGameState::SetBulletReady(bool bReady)
{
	BulletState = bReady ? EBulletState::Ready : EBulletState::Lost;
}

void AOBGameState::AddKill()
{
	++KillCount;
	BestKillCount = FMath::Max(BestKillCount, KillCount);
	GOneBulletBestKillCount = FMath::Max(GOneBulletBestKillCount, BestKillCount);
}

void AOBGameState::SetGameOver(bool bNewGameOver)
{
	SetGameOverWithReason(bNewGameOver, DeathReason);
}

void AOBGameState::SetGameOverWithReason(bool bNewGameOver, const FText& NewDeathReason)
{
	if (bNewGameOver && !bGameOver)
	{
		LastRunTime = SurvivalTime;
		BestKillCount = FMath::Max(BestKillCount, KillCount);
		GOneBulletBestKillCount = FMath::Max(GOneBulletBestKillCount, BestKillCount);
		DeathReason = NewDeathReason;
	}
	else if (!bNewGameOver)
	{
		SurvivalTime = 0.0f;
		LastRunTime = 0.0f;
		DeathReason = FText::GetEmpty();
	}

	bGameOver = bNewGameOver;
}

void AOBGameState::ResetRunState()
{
	BulletState = EBulletState::Ready;
	KillCount = 0;
	bGameOver = false;
	SurvivalTime = 0.0f;
	LastRunTime = 0.0f;
	DeathReason = FText::GetEmpty();
	BestKillCount = GOneBulletBestKillCount;
}
