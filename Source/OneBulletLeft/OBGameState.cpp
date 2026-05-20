#include "OBGameState.h"

AOBGameState::AOBGameState()
{
	BulletState = EBulletState::Ready;
	KillCount = 0;
	bGameOver = false;
}

void AOBGameState::SetBulletReady(bool bReady)
{
	BulletState = bReady ? EBulletState::Ready : EBulletState::Lost;
}

void AOBGameState::AddKill()
{
	++KillCount;
}

void AOBGameState::SetGameOver(bool bNewGameOver)
{
	bGameOver = bNewGameOver;
}
