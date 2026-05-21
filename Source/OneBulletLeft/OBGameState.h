#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "OBGameState.generated.h"

UENUM(BlueprintType)
enum class EBulletState : uint8
{
	Ready,
	Lost
};

UCLASS()
class ONEBULLETLEFT_API AOBGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AOBGameState();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet")
	EBulletState BulletState = EBulletState::Ready;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet")
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet")
	bool bGameOver = false;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet")
	float SurvivalTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet")
	float LastRunTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet")
	int32 BestKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet")
	FText DeathReason = FText::GetEmpty();

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	bool HasBullet() const { return BulletState == EBulletState::Ready; }

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void SetBulletReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void AddKill();

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void SetGameOver(bool bNewGameOver);

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void SetGameOverWithReason(bool bNewGameOver, const FText& NewDeathReason);

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void ResetRunState();
};
