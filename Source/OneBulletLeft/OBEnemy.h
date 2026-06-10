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

UENUM(BlueprintType)
enum class EOBEnemyAIState : uint8
{
	Cautious,
	Rush
};

UENUM(BlueprintType)
enum class EOBEnemyRole : uint8
{
	Chaser,
	Flanker,
	BulletBlocker
};

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AOBEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	static bool IsDetectionRadiusVisualizationEnabled();
	static void SetDetectionRadiusVisualizationEnabled(bool bEnabled);
	static void ToggleDetectionRadiusVisualization();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	EOBEnemyType EnemyType = EOBEnemyType::Fast;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float FastSpeed = 760.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings")
	float HeavySpeed = 260.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|AI State")
	EOBEnemyAIState CurrentAIState = EOBEnemyAIState::Cautious;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|AI State")
	EOBEnemyRole CurrentRole = EOBEnemyRole::Flanker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Transition", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float RushTransitionDelayMin = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Transition", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float RushTransitionDelayMax = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CautiousChaserChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CautiousFlankerChance = 0.80f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0"))
	float CautiousSpeedMultiplierMin = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0"))
	float CautiousSpeedMultiplierMax = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", UIMin="0.0", UIMax="0.5"))
	float CautiousSpeedRandomVariance = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", UIMin="100.0", UIMax="2000.0"))
	float CautiousFlankerDistance = 820.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", UIMin="100.0", UIMax="1200.0"))
	float CautiousFlankerMinDistance = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", UIMin="0.0", UIMax="150.0"))
	float CautiousCompressionSpeed = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RushChaserChance = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RushFlankerChance = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", ClampMax="1.0"))
	float RushBulletBlockerChance = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0"))
	float RushSpeedMultiplierMin = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", UIMin="0.0", UIMax="2.0"))
	float RushSpeedMultiplierMax = 1.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", UIMin="0.0", UIMax="0.5"))
	float RushSpeedRandomVariance = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", UIMin="50.0", UIMax="1000.0"))
	float RushFlankerDistance = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", UIMin="0.0", UIMax="500.0"))
	float BulletBlockerAcceptanceRadius = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.05", ClampMax="0.95", UIMin="0.25", UIMax="0.75"))
	float BulletBlockerPathFraction = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0", ClampMax="1.0"))
	float PlayerHasBulletSpeedMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0", ClampMax="1.0", DisplayName="Player Has Bullet Attack Speed Multiplier"))
	float PlayerHasBulletAttackSpeedMultiplier = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.0", DisplayName="Player Has Bullet Attack Radius"))
	float PlayerHasBulletAttackRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Detection", meta=(ClampMin="0.0"))
	float DetectionRadius = 2200.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="0.1", ClampMax="1.0"))
	float PatrolPerimeterRadiusMultiplier = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pressure", meta=(ClampMin="5.0", ClampMax="180.0"))
	float PatrolPerimeterStepDegrees = 45.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<class USoundBase> SpawnSound;

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
	float RunAnimationMinSpeed = 340.0f;

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

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|AI State")
	EOBEnemyAIState GetAIState() const { return CurrentAIState; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|AI State")
	EOBEnemyRole GetAIRole() const { return CurrentRole; }

	AOBBulletPickup* GetDroppedBulletPickup() const { return DroppedBulletPickup; }

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyDeath(const FVector& DropLocation);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyKicked(const FVector& Direction, EOBEnemyType Type);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemySpawned(EOBEnemyType Type, const FVector& Location);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyDisappearing(EOBEnemyType Type, const FVector& Location);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyAIStateChanged(EOBEnemyAIState NewState, EOBEnemyRole NewRole);

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
	bool bRushTransitionPending = false;
	bool bKickKnockbackActive = false;
	FVector PatrolOrigin = FVector::ZeroVector;
	FVector CurrentPatrolTarget = FVector::ZeroVector;
	FVector CurrentApproachTarget = FVector::ZeroVector;
	FVector LastPatrolLocation = FVector::ZeroVector;
	FVector ActiveKickKnockbackDirection = FVector::ZeroVector;
	TArray<FVector> CurrentPatrolPath;
	int32 CurrentPatrolPathIndex = 0;
	float PatrolStuckTime = 0.0f;
	float CurrentStateSpeedMultiplier = 1.0f;
	float CurrentStateElapsed = 0.0f;
	float FlankerSlotAngleDegrees = 90.0f;
	float ActiveKickKnockbackElapsed = 0.0f;
	float ActiveKickKnockbackDuration = 0.0f;
	float ActiveKickKnockbackDistance = 0.0f;
	float ActiveKickKnockbackPreviousAlpha = 0.0f;
	float ActiveLocomotionPlayRate = -1.0f;
	UAnimationAsset* ActiveLocomotionAnimation = nullptr;

	FTimerHandle StunTimerHandle;
	FTimerHandle MoveTimerHandle;
	FTimerHandle RushTransitionTimerHandle;

	void ResumeAfterStun();
	void BeginKickKnockback(const FVector& Direction, float Distance, float Duration, float StunDuration);
	void UpdateKickKnockback(float DeltaSeconds);
	void RequestMove();
	void StopPursuitForPlayerDeath();
	void NormalizePressureSettings();
	void RefreshAIStateFromBullet();
	void ScheduleRushTransition();
	void CompleteRushTransition();
	void SetAIState(EOBEnemyAIState NewState, bool bForceRefresh = false);
	void AssignRoleForCurrentState();
	void ApplyAIStateSpeed();
	void ResetMovementForStateChange();
	FVector CalculateStateMovementTarget() const;
	FVector CalculateCautiousTarget() const;
	FVector CalculateRushTarget() const;
	AOBBulletPickup* FindActiveBulletPickup() const;
	const TCHAR* GetAIStateName(EOBEnemyAIState State) const;
	const TCHAR* GetAIRoleName(EOBEnemyRole AIRole) const;
	bool IsPlayerHoldingBullet() const;
	bool IsPlayerInsideBulletAttackRadius() const;
	bool IsPlayerDetected() const;
	bool ShouldPatrolWhilePlayerHasBullet() const;
	FVector GetOrChoosePatrolTarget();
	FVector ChooseWholeArenaPatrolTarget() const;
	bool ProjectPointToNavigation(FVector& InOutLocation) const;
	bool IsPatrolCandidateClear(const FVector& Candidate) const;
	bool TryGetPatrolSteeringTarget(FVector& OutTarget) const;
	FVector CalculatePatrolMovementDirection(const FVector& DesiredDirection) const;
	bool MoveToCurrentTarget(float AcceptanceRadius, bool bAllowDirectFallback, bool bAllowPartialPath);
	bool RebuildPatrolPath();
	void UpdatePatrolMovement(float DeltaSeconds);
	void ChooseNewPatrolTarget();
	void MoveAggressivelyToPlayer();
	void ApplyDirectLostBulletChase();
	void UpdateSimpleLocomotionAnimation();
	FVector CalculateApproachTarget() const;
	void DrawDetectionRadiusDebug() const;
	void TryTouchKill();
};
