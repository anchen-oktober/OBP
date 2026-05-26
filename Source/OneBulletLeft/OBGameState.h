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

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AOBGameState();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings")
	EBulletState BulletState = EBulletState::Ready;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings")
	int32 KillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings")
	bool bGameOver = false;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings")
	float SurvivalTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings")
	float LastRunTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings")
	int32 BestKillCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings")
	FText DeathReason = FText::GetEmpty();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	bool HasBullet() const { return BulletState == EBulletState::Ready; }

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void SetBulletReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void AddKill();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void SetGameOver(bool bNewGameOver);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void SetGameOverWithReason(bool bNewGameOver, const FText& NewDeathReason);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void ResetRunState();
};
