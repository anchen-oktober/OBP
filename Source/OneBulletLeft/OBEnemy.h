#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OBEnemy.generated.h"

UENUM(BlueprintType)
enum class EOBEnemyType : uint8
{
	Fast,
	Heavy
};

UCLASS()
class ONEBULLETLEFT_API AOBEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AOBEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet")
	EOBEnemyType EnemyType = EOBEnemyType::Fast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet")
	float FastSpeed = 760.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet")
	float HeavySpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet")
	float TouchKillRadius = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet")
	float TouchKillExtraMargin = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet")
	bool bCanTouchKillFromBehind = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet", meta=(EditCondition="!bCanTouchKillFromBehind", ClampMin="-1.0", ClampMax="1.0"))
	float TouchKillFrontMinDot = -0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Audio")
	TObjectPtr<class USoundBase> DeathSound;

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void Configure(EOBEnemyType NewType);

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void KillAndDropBullet(const FVector& DropLocation);

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void ApplyKick(const FVector& Direction);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|Events")
	void OnEnemyDeath(const FVector& DropLocation);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|Events")
	void OnEnemyKicked(const FVector& Direction, EOBEnemyType Type);

protected:
	UPROPERTY()
	TObjectPtr<class AOBCharacter> PlayerTarget;

	bool bDead = false;
	bool bStunned = false;

	FTimerHandle StunTimerHandle;
	FTimerHandle MoveTimerHandle;

	void ResumeAfterStun();
	void RequestMove();
	void TryTouchKill();
};
