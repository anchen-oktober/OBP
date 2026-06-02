#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OBEnemy.generated.h"

class AOBBulletPickup;
class UAnimationAsset;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PlayerHasBulletSpeedMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0", ClampMax="1.0", DisplayName="Player Has Bullet Attack Speed Multiplier"))
	float PlayerHasBulletAttackSpeedMultiplier = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0", DisplayName="Player Has Bullet Attack Radius"))
	float PlayerHasBulletAttackRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0"))
	float PatrolRadius = 2600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure")
	FVector PatrolCenter = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0"))
	float PatrolPointJitter = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0"))
	float PatrolMinTargetDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="1.0"))
	float PatrolAcceptanceRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0"))
	float PatrolObstacleProbeDistance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0"))
	float PatrolObstacleProbeRadius = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float TouchKillRadius = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float TouchKillExtraMargin = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure")
	bool bUseSurroundMovement = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(EditCondition="bUseSurroundMovement", ClampMin="0.0", ClampMax="180.0"))
	float SurroundFrontArcDegrees = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(EditCondition="bUseSurroundMovement", ClampMin="0.0"))
	float FastSurroundRadius = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(EditCondition="bUseSurroundMovement", ClampMin="0.0"))
	float HeavySurroundRadius = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(EditCondition="bUseSurroundMovement", ClampMin="0.0"))
	float MinAggressiveSurroundRadius = 260.0f;

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
	TObjectPtr<UAnimationAsset> WalkAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> RunAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	bool bUseSimpleLocomotionAnimations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(EditCondition="bUseSimpleLocomotionAnimations", ClampMin="0.0"))
	float WalkAnimationMinSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(EditCondition="bUseSimpleLocomotionAnimations", ClampMin="0.0"))
	float RunAnimationMinSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(EditCondition="bUseSimpleLocomotionAnimations", ClampMin="0.0"))
	float WalkAnimationPlayRate = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(EditCondition="bUseSimpleLocomotionAnimations", ClampMin="0.0"))
	float RunAnimationPlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Bullet Attachment")
	FName BulletAttachBone = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Bullet Attachment")
	FVector BulletAttachOffset = FVector(0.0f, 0.0f, 16.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Movement")
	bool bUseDirectMovementFallback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Movement")
	bool bUseDirectLostBulletChase = true;

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
	void OnEnemySpawned(EOBEnemyType Type, const FVector& Location);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyDisappearing(EOBEnemyType Type, const FVector& Location);

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
	bool bMovingToPatrolTarget = false;
	bool bHasPatrolTarget = false;
	bool bWasHoldingBullet = true;
	FVector PatrolOrigin = FVector::ZeroVector;
	FVector CurrentPatrolTarget = FVector::ZeroVector;
	FVector CurrentApproachTarget = FVector::ZeroVector;
	FVector LastPatrolLocation = FVector::ZeroVector;
	float PatrolStuckTime = 0.0f;
	UAnimationAsset* ActiveLocomotionAnimation = nullptr;

	FTimerHandle StunTimerHandle;
	FTimerHandle MoveTimerHandle;

	void ResumeAfterStun();
	void RequestMove();
	void StopPursuitForPlayerDeath();
	void NormalizePressureSettings();
	void ApplyBulletPressureSpeed();
	bool IsPlayerHoldingBullet() const;
	bool IsPlayerInsideBulletAttackRadius() const;
	bool ShouldPatrolWhilePlayerHasBullet() const;
	FVector GetOrChoosePatrolTarget();
	FVector ChooseWholeArenaPatrolTarget() const;
	bool ProjectPointToNavigation(FVector& InOutLocation) const;
	bool TryGetPatrolSteeringTarget(FVector& OutTarget) const;
	FVector CalculatePatrolMovementDirection(const FVector& DesiredDirection) const;
	bool MoveToCurrentTarget(float AcceptanceRadius, bool bAllowDirectFallback, bool bAllowPartialPath);
	void UpdatePatrolMovement(float DeltaSeconds);
	void ChooseNewPatrolTarget();
	void MoveAggressivelyToPlayer();
	void ApplyDirectLostBulletChase();
	void UpdateSimpleLocomotionAnimation();
	FVector CalculateApproachTarget() const;
	void TryTouchKill();
};
