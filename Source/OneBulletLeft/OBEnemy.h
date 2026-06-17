#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OBEnemy.generated.h"

class AOBBulletPickup;
class AOBEnemy;
class UAnimationAsset;
class UDecalComponent;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOBEnemyDeathReportedSignature, AOBEnemy*, Enemy);

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

UENUM(BlueprintType)
enum class EOBBulletGuardPriority : uint8
{
	None,
	Attack,
	Intercept,
	Guard
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
	float CautiousFlankerDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Cautious", meta=(ClampMin="0.0", UIMin="100.0", UIMax="1200.0"))
	float CautiousFlankerMinDistance = 300.0f;

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
	float RushFlankerDistanceMin = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", UIMin="50.0", UIMax="1000.0"))
	float RushFlankerDistanceMax = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Rush", meta=(ClampMin="0.0", UIMin="0.0", UIMax="500.0"))
	float BulletBlockerAcceptanceRadius = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Bullet Guard", meta=(ClampMin="250.0", ClampMax="400.0", UIMin="250.0", UIMax="400.0"))
	float BulletGuardRadius = 325.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Bullet Guard", meta=(ClampMin="500.0", ClampMax="800.0", UIMin="500.0", UIMax="800.0"))
	float BulletGuardInterceptRadius = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Bullet Guard", meta=(ClampMin="600.0", ClampMax="900.0", UIMin="600.0", UIMax="900.0"))
	float PlayerNearBulletRadius = 750.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Bullet Guard", meta=(ClampMin="500.0", ClampMax="700.0", UIMin="500.0", UIMax="700.0"))
	float MaxGuardDistanceFromBullet = 600.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|AI State|Bullet Guard")
	EOBBulletGuardPriority CurrentBulletGuardPriority = EOBBulletGuardPriority::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Debug")
	bool bDrawAIRoleTargets = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Debug", meta=(EditCondition="bDrawAIRoleTargets", ClampMin="2.0", UIMin="4.0", UIMax="30.0"))
	float AIRoleTargetDebugSize = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Movement Diversity", meta=(ClampMin="0.0", UIMin="100.0", UIMax="800.0"))
	float MinDistanceBetweenEnemyTargets = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Movement Diversity", meta=(ClampMin="0.0", UIMin="100.0", UIMax="1500.0"))
	float FlankRadiusMin = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Movement Diversity", meta=(ClampMin="0.0", UIMin="100.0", UIMax="1800.0"))
	float FlankRadiusMax = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Movement Diversity", meta=(ClampMin="0.0", UIMin="0.0", UIMax="500.0"))
	float TargetRandomOffsetMin = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Movement Diversity", meta=(ClampMin="0.0", UIMin="0.0", UIMax="600.0"))
	float TargetRandomOffsetMax = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Movement Diversity", meta=(ClampMin="1", ClampMax="20"))
	int32 TargetReservationAttempts = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="100.0", UIMax="1500.0"))
	float CautiousFlankStartRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="100.0", UIMax="1000.0"))
	float CautiousFlankMinRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="500.0"))
	float CautiousFlankApproachSpeedMin = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="500.0"))
	float CautiousFlankApproachSpeedMax = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="100.0", UIMax="1200.0"))
	float RushFlankStartRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="50.0", UIMax="800.0"))
	float RushFlankMinRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="700.0"))
	float RushFlankApproachSpeedMin = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="700.0"))
	float RushFlankApproachSpeedMax = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="10.0"))
	float FlankSideLockDurationMin = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="10.0"))
	float FlankSideLockDurationMax = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="400.0"))
	float MaxAllowedFlankDistanceIncrease = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="300.0"))
	float FlankTargetLateralOffsetMax = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Flanker", meta=(ClampMin="0.0", UIMin="0.0", UIMax="300.0"))
	float FlankClosePressureRange = 75.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Repath", meta=(ClampMin="0.05", UIMin="0.05", UIMax="2.0"))
	float ChaserRepathIntervalMin = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Repath", meta=(ClampMin="0.05", UIMin="0.05", UIMax="2.0"))
	float ChaserRepathIntervalMax = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Repath", meta=(ClampMin="0.05", UIMin="0.05", UIMax="3.0"))
	float FlankerRepathIntervalMin = 0.80f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Repath", meta=(ClampMin="0.05", UIMin="0.05", UIMax="3.0"))
	float FlankerRepathIntervalMax = 1.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Repath", meta=(ClampMin="0.05", UIMin="0.05", UIMax="2.0"))
	float BlockerRepathIntervalMin = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|AI State|Repath", meta=(ClampMin="0.05", UIMin="0.05", UIMax="2.0"))
	float BlockerRepathIntervalMax = 0.80f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Attack Radius", meta=(ClampMin="0.0", UIMin="50.0", UIMax="300.0"))
	float FastAttackRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Attack Radius", meta=(ClampMin="0.0", UIMin="100.0", UIMax="500.0"))
	float HeavyAttackRadius = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Attack Radius")
	float TouchKillExtraMargin = 12.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Attack Radius")
	TObjectPtr<UDecalComponent> HeavyAttackAura;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Attack Radius", meta=(ClampMin="0.0", ClampMax="1.0"))
	float HeavyAttackAuraOpacity = 0.11f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Attack Radius", meta=(ClampMin="0.0", ClampMax="1.0"))
	float HeavyAttackAuraReadyOpacity = 0.15f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation|Locomotion", meta=(ClampMin="1.0", UIMin="90.0", UIMax="540.0"))
	float RotationRateYaw = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation|Locomotion", meta=(ClampMin="0.0", UIMin="1.0", UIMax="20.0"))
	float DirectionInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation|Locomotion", meta=(ClampMin="0.0", UIMin="1.0", UIMax="20.0"))
	float TurnAngleInterpSpeed = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation|Locomotion", meta=(ClampMin="0.0", UIMin="1.0", UIMax="30.0"))
	float AccelerationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation|Locomotion", meta=(ClampMin="0.0", UIMin="0.0", UIMax="100.0"))
	float MovingSpeedThreshold = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation|Locomotion", meta=(ClampMin="0.0", ClampMax="180.0", UIMin="5.0", UIMax="120.0"))
	float TurningAngleThreshold = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation|Locomotion", meta=(ClampMin="0.0", UIMin="0.0", UIMax="150.0"))
	float TurnInPlaceMaxSpeed = 35.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="OneBulletSettings|Animation|Locomotion")
	float Speed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="OneBulletSettings|Animation|Locomotion")
	float Direction = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="OneBulletSettings|Animation|Locomotion")
	FVector Acceleration = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="OneBulletSettings|Animation|Locomotion")
	float YawDelta = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="OneBulletSettings|Animation|Locomotion")
	float TurnAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="OneBulletSettings|Animation|Locomotion")
	bool bIsMoving = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="OneBulletSettings|Animation|Locomotion")
	bool bIsTurning = false;

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

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Difficulty")
	void SetDifficultySpeedMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Difficulty")
	float GetDifficultySpeedMultiplier() const { return DifficultySpeedMultiplier; }

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void KillAndDropBullet(const FVector& DropLocation);

	UPROPERTY(BlueprintAssignable, Category="OneBulletSettings|Events")
	FOBEnemyDeathReportedSignature OnDeathReported;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void ApplyKick(const FVector& KickDirection);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Relief")
	void ApplyBulletPickupReliefReaction(AActor* PlayerActor, float SlowDuration, float SpeedMultiplier, float StepBackDistance, float StepBackDuration);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Events")
	void TriggerSpawnFeedback();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Spawning")
	void BeginSpawnProtection(float WarningDuration, float GracePeriod);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Spawning")
	bool IsSpawnProtected() const { return bSpawnProtected; }

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Events")
	void Disappear();

	UFUNCTION(BlueprintPure, Category="OneBulletSettings")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|AI State")
	EOBEnemyAIState GetAIState() const { return CurrentAIState; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|AI State")
	EOBEnemyRole GetAIRole() const { return CurrentRole; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Attack Radius")
	float GetEffectiveAttackRadius() const;

	AOBBulletPickup* GetDroppedBulletPickup() const { return DroppedBulletPickup; }

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyDeath(const FVector& DropLocation);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnEnemyKicked(const FVector& KickDirection, EOBEnemyType Type);

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
	bool bHasFlankSetup = false;
	bool bSpawnProtected = false;
	bool bSpawnWarningActive = false;
	FVector PatrolOrigin = FVector::ZeroVector;
	FVector CurrentPatrolTarget = FVector::ZeroVector;
	FVector CurrentApproachTarget = FVector::ZeroVector;
	FVector LastPatrolLocation = FVector::ZeroVector;
	FVector ActiveKickKnockbackDirection = FVector::ZeroVector;
	FVector LockedFlankDirection = FVector::RightVector;
	TArray<FVector> CurrentPatrolPath;
	int32 CurrentPatrolPathIndex = 0;
	float PatrolStuckTime = 0.0f;
	float CurrentStateSpeedMultiplier = 1.0f;
	float DifficultySpeedMultiplier = 1.0f;
	float ReliefSpeedMultiplier = 1.0f;
	float CurrentStateElapsed = 0.0f;
	float FlankerSlotAngleDegrees = 90.0f;
	float CurrentFlankerOffsetDistance = 450.0f;
	float CurrentFlankApproachSpeed = 100.0f;
	float FlankSideLockRemaining = 0.0f;
	float LockedFlankLateralOffset = 0.0f;
	float RepathTimeJitter = 1.0f;
	float BulletGuardDistanceToPlayer = 0.0f;
	float BulletGuardPlayerToBulletDistance = 0.0f;
	EOBEnemyAIState FlankerRadiusState = EOBEnemyAIState::Cautious;
	float ActiveKickKnockbackElapsed = 0.0f;
	float ActiveKickKnockbackDuration = 0.0f;
	float ActiveKickKnockbackDistance = 0.0f;
	float ActiveKickKnockbackPreviousAlpha = 0.0f;
	float ActiveLocomotionPlayRate = -1.0f;
	UAnimationAsset* ActiveLocomotionAnimation = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeavyAttackAuraMaterial;

	FTimerHandle StunTimerHandle;
	FTimerHandle MoveTimerHandle;
	FTimerHandle RushTransitionTimerHandle;
	FTimerHandle SpawnWarningTimerHandle;
	FTimerHandle SpawnGraceTimerHandle;
	FTimerHandle ReliefReactionTimerHandle;

	void ResumeAfterStun();
	void FinishBulletPickupReliefReaction();
	void FinishSpawnWarning();
	void FinishSpawnProtection();
	void BeginKickKnockback(const FVector& KnockbackDirection, float Distance, float Duration, float StunDuration);
	void UpdateKickKnockback(float DeltaSeconds);
	void RequestMove();
	void StopPursuitForPlayerDeath();
	void NormalizePressureSettings();
	void RefreshAIStateFromBullet();
	void ScheduleRushTransition();
	void CompleteRushTransition();
	void SetAIState(EOBEnemyAIState NewState, bool bForceRefresh = false);
	void AssignEnemyRoles();
	void SetAssignedRole(EOBEnemyRole NewRole, int32 RoleSlot, int32 RoleCount);
	bool UpdateBulletGuardPriority(bool bRefreshMovementOnChange = false);
	void ApplyAIStateSpeed();
	void ResetMovementForStateChange();
	void ScheduleNextMoveRequest(float InitialDelay = -1.0f);
	float GetNextRepathInterval() const;
	FVector CalculateStateMovementTarget();
	FVector CalculateChaserTarget() const;
	FVector CalculateFlankerTarget(bool bCompressDistance);
	FVector CalculateBulletBlockerTarget();
	FVector FindUnclaimedTarget(const TFunction<FVector(int32)>& CandidateGenerator, const FVector& FallbackTarget);
	bool IsTargetClaimedByAnotherEnemy(const FVector& Candidate) const;
	void GetLiveEnemies(TArray<AOBEnemy*>& OutEnemies) const;
	FVector GetPlayerMovementDirection() const;
	AOBBulletPickup* FindActiveBulletPickup() const;
	void DrawAIRoleTargetDebug() const;
	const TCHAR* GetAIStateName(EOBEnemyAIState State) const;
	const TCHAR* GetAIRoleName(EOBEnemyRole AIRole) const;
	const TCHAR* GetBulletGuardPriorityName(EOBBulletGuardPriority Priority) const;
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
	void UpdateSmoothFacing(float DeltaSeconds);
	void UpdateLocomotionState(float DeltaSeconds);
	void UpdateSimpleLocomotionAnimation();
	FVector CalculateApproachTarget() const;
	void DrawDetectionRadiusDebug() const;
	void RefreshHeavyAttackAura();
	void TryTouchKill();
};
