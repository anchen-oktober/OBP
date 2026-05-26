#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OBEnemy.generated.h"

class AOBBulletPickup;
class UAnimationAsset;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EOBEnemyType : uint8
{
	Fast,
	Heavy
};

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AOBEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	EOBEnemyType EnemyType = EOBEnemyType::Fast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float FastSpeed = 760.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float HeavySpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float TouchKillRadius = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float TouchKillExtraMargin = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure")
	bool bUseSurroundMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(EditCondition="bUseSurroundMovement", ClampMin="0.0", ClampMax="180.0"))
	float SurroundFrontArcDegrees = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(EditCondition="bUseSurroundMovement", ClampMin="0.0"))
	float FastSurroundRadius = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(EditCondition="bUseSurroundMovement", ClampMin="0.0"))
	float HeavySurroundRadius = 104.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	bool bCanTouchKillFromBehind = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings", meta=(EditCondition="!bCanTouchKillFromBehind", ClampMin="-1.0", ClampMax="1.0"))
	float TouchKillFrontMinDot = -0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<class USoundBase> DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Feel")
	float ShotStaggerStrength = 340.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Feel")
	float DeathFeedbackDuration = 0.13f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> DeathAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> RunAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	bool bUseSimpleLocomotionAnimations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(EditCondition="bUseSimpleLocomotionAnimations", ClampMin="0.0"))
	float RunAnimationMinSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Bullet Attachment")
	FName BulletAttachBone = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Bullet Attachment")
	FVector BulletAttachOffset = FVector(0.0f, 0.0f, 16.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Movement")
	bool bUseDirectMovementFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|VFX")
	TObjectPtr<UNiagaraSystem> SpawnEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|VFX")
	TObjectPtr<UNiagaraSystem> DisappearEffect;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void Configure(EOBEnemyType NewType);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void KillAndDropBullet(const FVector& DropLocation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void ApplyKick(const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Events")
	void TriggerSpawnFeedback();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Events")
	void Disappear();

	UFUNCTION(BlueprintPure, Category="OneBulletSettings")
	bool IsDead() const { return bDead; }

	AOBBulletPickup* GetDroppedBulletPickup() const { return DroppedBulletPickup; }

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyDeath(const FVector& DropLocation);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyKicked(const FVector& Direction, EOBEnemyType Type);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemySpawned(EOBEnemyType Type);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyDisappearing(EOBEnemyType Type);

protected:
	UPROPERTY()
	TObjectPtr<class AOBCharacter> PlayerTarget;

	UPROPERTY()
	TObjectPtr<AOBBulletPickup> DroppedBulletPickup;

	bool bDead = false;
	bool bStunned = false;
	bool bDisappearing = false;
	bool bUsingDirectMovementFallback = false;
	bool bStoppedForPlayerDeath = false;
	FVector CurrentApproachTarget = FVector::ZeroVector;
	UAnimationAsset* ActiveLocomotionAnimation = nullptr;

	FTimerHandle StunTimerHandle;
	FTimerHandle MoveTimerHandle;

	void ResumeAfterStun();
	void RequestMove();
	void StopPursuitForPlayerDeath();
	void UpdateSimpleLocomotionAnimation();
	FVector CalculateApproachTarget() const;
	void TryTouchKill();
};
