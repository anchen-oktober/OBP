#include "OBEnemy.h"

#include "AIController.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "OBCharacter.h"
#include "OBBulletPickup.h"
#include "OBGameMode.h"
#include "OBGameState.h"

DEFINE_LOG_CATEGORY_STATIC(LogOBEnemyAI, Log, All);

namespace
{
bool IsAnimationCompatibleWithMesh(const UAnimationAsset* Animation, const USkeletalMeshComponent* MeshComponent)
{
	const USkeletalMesh* SkeletalMesh = MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr;
	return !Animation || !SkeletalMesh || !Animation->GetSkeleton() || Animation->GetSkeleton() == SkeletalMesh->GetSkeleton();
}

bool bShowEnemyDetectionRadii = false;
TWeakObjectPtr<UWorld> DetectionVisualizationWorld;
}

AOBEnemy::AOBEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(44.0f, 92.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = FastSpeed;
	GetCharacterMovement()->BrakingFrictionFactor = 0.4f;

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -92.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAnimBP(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (UnarmedAnimBP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(UnarmedAnimBP.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DefaultDeathAnimation(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
	if (DefaultDeathAnimation.Succeeded())
	{
		DeathAnimation = DefaultDeathAnimation.Object;
	}
}

void AOBEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (!GetController())
	{
		SpawnDefaultController();
	}
	UWorld* World = GetWorld();
	if (World && DetectionVisualizationWorld.Get() != World)
	{
		DetectionVisualizationWorld = World;
		bShowEnemyDetectionRadii = false;
	}
	NormalizePressureSettings();
	Configure(EnemyType);
	PlayerTarget = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	PatrolOrigin = PatrolCenter;
	PatrolOrigin.Z = GetActorLocation().Z;
	LastPatrolLocation = PatrolOrigin;
	bWasHoldingBullet = IsPlayerHoldingBullet();
	RepathTimeJitter = FMath::FRandRange(0.90f, 1.10f);
	SetAIState(bWasHoldingBullet ? EOBEnemyAIState::Cautious : EOBEnemyAIState::Rush, true);
	ScheduleNextMoveRequest(FMath::FRandRange(0.05f, 0.25f));
}

bool AOBEnemy::IsDetectionRadiusVisualizationEnabled()
{
	return bShowEnemyDetectionRadii;
}

void AOBEnemy::SetDetectionRadiusVisualizationEnabled(bool bEnabled)
{
	bShowEnemyDetectionRadii = bEnabled;
}

void AOBEnemy::ToggleDetectionRadiusVisualization()
{
	bShowEnemyDetectionRadii = !bShowEnemyDetectionRadii;
}

void AOBEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead)
	{
		if (!IsValid(DroppedBulletPickup))
		{
			Disappear();
		}
		return;
	}
	if (bSpawnWarningActive)
	{
		return;
	}

	CurrentStateElapsed += DeltaSeconds;
	if (CurrentRole == EOBEnemyRole::Flanker)
	{
		const float MinFlankRadius = CurrentAIState == EOBEnemyAIState::Cautious
			? CautiousFlankMinRadius
			: RushFlankMinRadius;
		CurrentFlankerOffsetDistance = FMath::Max(
			MinFlankRadius,
			CurrentFlankerOffsetDistance - CurrentFlankApproachSpeed * DeltaSeconds);
		FlankSideLockRemaining = FMath::Max(0.0f, FlankSideLockRemaining - DeltaSeconds);
	}
	RefreshAIStateFromBullet();

	if (PlayerTarget && PlayerTarget->IsDead())
	{
		StopPursuitForPlayerDeath();
		UpdateSimpleLocomotionAnimation();
		return;
	}

	bStoppedForPlayerDeath = false;
	UpdateKickKnockback(DeltaSeconds);
	if (!bStunned && PlayerTarget)
	{
		if (bUsingDirectMovementFallback)
		{
			const FVector MovementDirection = (CurrentApproachTarget - GetActorLocation()).GetSafeNormal2D();
			if (!MovementDirection.IsNearlyZero())
			{
				AddMovementInput(MovementDirection, 1.0f, true);
			}
		}

		const FVector FacingTarget = CurrentApproachTarget.IsNearlyZero()
			? PlayerTarget->GetActorLocation()
			: CurrentApproachTarget;
		const FVector FacingDirection = (FacingTarget - GetActorLocation()).GetSafeNormal2D();
		if (!FacingDirection.IsNearlyZero())
		{
			SetActorRotation(FacingDirection.Rotation());
		}
	}
	UpdateSimpleLocomotionAnimation();
	TryTouchKill();
	DrawDetectionRadiusDebug();
	DrawAttackRadiusDebug();
	DrawAIRoleTargetDebug();
}

void AOBEnemy::Configure(EOBEnemyType NewType)
{
	EnemyType = NewType;

	if (EnemyType == EOBEnemyType::Fast)
	{
		GetCapsuleComponent()->SetCapsuleSize(38.0f, 92.0f);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -92.0f));
		GetMesh()->SetRelativeScale3D(FVector(0.92f, 0.92f, 0.92f));
	}
	else
	{
		GetCapsuleComponent()->SetCapsuleSize(58.0f, 110.0f);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -110.0f));
		GetMesh()->SetRelativeScale3D(FVector(1.25f, 1.25f, 1.25f));
	}

	ApplyAIStateSpeed();
}

void AOBEnemy::SetDifficultySpeedMultiplier(float NewMultiplier)
{
	DifficultySpeedMultiplier = FMath::Max(NewMultiplier, 0.0f);
	ApplyAIStateSpeed();
}

void AOBEnemy::KillAndDropBullet(const FVector& DropLocation)
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	OnDeathReported.Broadcast(this);
	GetWorldTimerManager().ClearTimer(MoveTimerHandle);
	GetWorldTimerManager().ClearTimer(RushTransitionTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnWarningTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnGraceTimerHandle);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}

	if (DeathAnimation && IsAnimationCompatibleWithMesh(DeathAnimation, GetMesh()))
	{
		ActiveLocomotionAnimation = nullptr;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->PlayAnimation(DeathAnimation, false);
	}
	else if (DeathAnimation)
	{
		UE_LOG(LogTemp, Warning, TEXT("Skipping death animation %s: it uses a different skeleton than enemy %s."), *GetNameSafe(DeathAnimation), *GetName());
	}
	OnEnemyDeath(DropLocation);
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	if (AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>())
	{
		OneBulletState->AddKill();
	}

	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		DroppedBulletPickup = OneBulletMode->SpawnBulletPickup(DropLocation);
		if (DroppedBulletPickup && GetMesh())
		{
			DroppedBulletPickup->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, BulletAttachBone);
			DroppedBulletPickup->SetActorRelativeLocation(BulletAttachOffset);
		}
	}

	AssignEnemyRoles();
}

void AOBEnemy::ApplyKick(const FVector& Direction)
{
	if (bDead)
	{
		return;
	}

	const FVector LaunchDirection = Direction.GetSafeNormal2D();
	OnEnemyKicked(LaunchDirection, EnemyType);
	if (EnemyType == EOBEnemyType::Fast)
	{
		BeginKickKnockback(LaunchDirection, 560.0f, 0.26f, 0.35f);
	}
	else
	{
		BeginKickKnockback(LaunchDirection, 360.0f, 0.30f, 1.0f);
	}
}

void AOBEnemy::TriggerSpawnFeedback()
{
	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation());
	}

	OnEnemySpawned(EnemyType, GetActorLocation());
}

void AOBEnemy::BeginSpawnProtection(float WarningDuration, float GracePeriod)
{
	if (bDead || !GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnWarningTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnGraceTimerHandle);
	GetWorldTimerManager().ClearTimer(MoveTimerHandle);
	bSpawnProtected = true;
	bSpawnWarningActive = true;
	bUsingDirectMovementFallback = false;
	SetActorHiddenInGame(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}

	const float SafeWarningDuration = FMath::Max(WarningDuration, 0.0f);
	const float SafeGracePeriod = FMath::Max(GracePeriod, 0.0f);
	UE_LOG(
		LogOBEnemyAI,
		Log,
		TEXT("%s spawn warning started: hidden for %.2fs, grace %.2fs"),
		*GetName(),
		SafeWarningDuration,
		SafeGracePeriod);
	if (SafeWarningDuration <= KINDA_SMALL_NUMBER)
	{
		FinishSpawnWarning();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			SpawnWarningTimerHandle,
			this,
			&AOBEnemy::FinishSpawnWarning,
			SafeWarningDuration,
			false);
	}

	if (SafeGracePeriod <= KINDA_SMALL_NUMBER)
	{
		FinishSpawnProtection();
	}
	else
	{
		GetWorldTimerManager().SetTimer(
			SpawnGraceTimerHandle,
			this,
			&AOBEnemy::FinishSpawnProtection,
			SafeWarningDuration + SafeGracePeriod,
			false);
	}
}

void AOBEnemy::FinishSpawnWarning()
{
	if (bDead || !bSpawnWarningActive)
	{
		return;
	}

	bSpawnWarningActive = false;
	SetActorHiddenInGame(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	ApplyAIStateSpeed();
	TriggerSpawnFeedback();
	ScheduleNextMoveRequest(0.01f);
	UE_LOG(LogOBEnemyAI, Log, TEXT("%s became visible with spawn protection active"), *GetName());
}

void AOBEnemy::FinishSpawnProtection()
{
	bSpawnProtected = false;
	UE_LOG(LogOBEnemyAI, Log, TEXT("%s spawn protection ended"), *GetName());
}

void AOBEnemy::Disappear()
{
	if (bDisappearing)
	{
		return;
	}

	bDisappearing = true;
	GetWorldTimerManager().ClearTimer(SpawnWarningTimerHandle);
	GetWorldTimerManager().ClearTimer(SpawnGraceTimerHandle);
	OnEnemyDisappearing(EnemyType, GetActorLocation());
	Destroy();
}

void AOBEnemy::ResumeAfterStun()
{
	bStunned = false;
	RequestMove();
}

void AOBEnemy::BeginKickKnockback(const FVector& Direction, float Distance, float Duration, float StunDuration)
{
	ActiveKickKnockbackDirection = Direction.GetSafeNormal2D();
	if (ActiveKickKnockbackDirection.IsNearlyZero())
	{
		return;
	}

	bStunned = true;
	bKickKnockbackActive = true;
	ActiveKickKnockbackElapsed = 0.0f;
	ActiveKickKnockbackPreviousAlpha = 0.0f;
	ActiveKickKnockbackDistance = FMath::Max(0.0f, Distance);
	ActiveKickKnockbackDuration = FMath::Max(0.01f, Duration);
	GetCharacterMovement()->StopMovementImmediately();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
	GetWorldTimerManager().SetTimer(StunTimerHandle, this, &AOBEnemy::ResumeAfterStun, FMath::Max(StunDuration, ActiveKickKnockbackDuration), false);
}

void AOBEnemy::UpdateKickKnockback(float DeltaSeconds)
{
	if (!bKickKnockbackActive)
	{
		return;
	}

	ActiveKickKnockbackElapsed += DeltaSeconds;
	const float RawAlpha = FMath::Clamp(ActiveKickKnockbackElapsed / ActiveKickKnockbackDuration, 0.0f, 1.0f);
	const float SmoothedAlpha = RawAlpha * RawAlpha * (3.0f - 2.0f * RawAlpha);
	const float AlphaStep = SmoothedAlpha - ActiveKickKnockbackPreviousAlpha;
	ActiveKickKnockbackPreviousAlpha = SmoothedAlpha;

	const float PreviousZ = GetActorLocation().Z;
	FHitResult Hit;
	AddActorWorldOffset(ActiveKickKnockbackDirection * ActiveKickKnockbackDistance * AlphaStep, true, &Hit);
	const FVector NewLocation = GetActorLocation();
	if (!FMath::IsNearlyEqual(NewLocation.Z, PreviousZ, 0.1f))
	{
		SetActorLocation(FVector(NewLocation.X, NewLocation.Y, PreviousZ), false);
	}
	if (RawAlpha >= 1.0f || Hit.bBlockingHit)
	{
		bKickKnockbackActive = false;
		ActiveKickKnockbackDirection = FVector::ZeroVector;
		ActiveKickKnockbackElapsed = 0.0f;
		ActiveKickKnockbackPreviousAlpha = 0.0f;
	}
}

void AOBEnemy::StopPursuitForPlayerDeath()
{
	if (bStoppedForPlayerDeath)
	{
		return;
	}

	bStoppedForPlayerDeath = true;
	bUsingDirectMovementFallback = false;
	bMovingToPatrolTarget = false;
	GetCharacterMovement()->StopMovementImmediately();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
}

void AOBEnemy::NormalizePressureSettings()
{
	PlayerHasBulletSpeedMultiplier = FMath::Clamp(PlayerHasBulletSpeedMultiplier, 0.0f, 1.0f);
	PlayerHasBulletAttackSpeedMultiplier = FMath::Clamp(PlayerHasBulletAttackSpeedMultiplier, 0.0f, 1.0f);
	PlayerHasBulletAttackRadius = FMath::Max(PlayerHasBulletAttackRadius, 0.0f);
	DetectionRadius = FMath::Max(DetectionRadius, 0.0f);
	PatrolRadius = FMath::Max(PatrolRadius, 2600.0f);
	PatrolPointJitter = FMath::Max(PatrolPointJitter, 0.0f);
	PatrolMinTargetDistance = FMath::Clamp(PatrolMinTargetDistance, PatrolAcceptanceRadius, PatrolRadius * 0.75f);
	PatrolPerimeterRadiusMultiplier = FMath::Clamp(PatrolPerimeterRadiusMultiplier, 0.1f, 1.0f);
	PatrolPerimeterStepDegrees = FMath::Clamp(PatrolPerimeterStepDegrees, 5.0f, 180.0f);
	PatrolObstacleProbeDistance = FMath::Max(PatrolObstacleProbeDistance, 0.0f);
	PatrolObstacleProbeRadius = FMath::Max(PatrolObstacleProbeRadius, 0.0f);
	FastAttackRadius = FMath::Max(FastAttackRadius, 0.0f);
	HeavyAttackRadius = FMath::Max(HeavyAttackRadius, FastAttackRadius);
	AttackRadiusDebugThickness = FMath::Max(AttackRadiusDebugThickness, 1.0f);

	RushTransitionDelayMin = FMath::Max(RushTransitionDelayMin, 0.0f);
	RushTransitionDelayMax = FMath::Max(RushTransitionDelayMax, RushTransitionDelayMin);
	CautiousChaserChance = FMath::Clamp(CautiousChaserChance, 0.0f, 1.0f);
	CautiousFlankerChance = FMath::Clamp(CautiousFlankerChance, 0.0f, 1.0f);
	CautiousSpeedMultiplierMin = FMath::Max(CautiousSpeedMultiplierMin, 0.0f);
	CautiousSpeedMultiplierMax = FMath::Max(CautiousSpeedMultiplierMax, CautiousSpeedMultiplierMin);
	CautiousSpeedRandomVariance = FMath::Clamp(CautiousSpeedRandomVariance, 0.0f, 0.5f);
	CautiousFlankerDistance = FMath::Max(CautiousFlankerDistance, 0.0f);
	CautiousFlankerMinDistance = FMath::Clamp(CautiousFlankerMinDistance, 0.0f, CautiousFlankerDistance);
	CautiousCompressionSpeed = FMath::Max(CautiousCompressionSpeed, 0.0f);
	RushChaserChance = FMath::Clamp(RushChaserChance, 0.0f, 1.0f);
	RushFlankerChance = FMath::Clamp(RushFlankerChance, 0.0f, 1.0f);
	RushBulletBlockerChance = FMath::Clamp(RushBulletBlockerChance, 0.0f, 1.0f);
	RushSpeedMultiplierMin = FMath::Max(RushSpeedMultiplierMin, 0.0f);
	RushSpeedMultiplierMax = FMath::Max(RushSpeedMultiplierMax, RushSpeedMultiplierMin);
	RushSpeedRandomVariance = FMath::Clamp(RushSpeedRandomVariance, 0.0f, 0.5f);
	RushFlankerDistanceMin = FMath::Max(RushFlankerDistanceMin, 0.0f);
	RushFlankerDistanceMax = FMath::Max(RushFlankerDistanceMax, RushFlankerDistanceMin);
	BulletBlockerAcceptanceRadius = FMath::Max(BulletBlockerAcceptanceRadius, 0.0f);
	AIRoleTargetDebugSize = FMath::Max(AIRoleTargetDebugSize, 2.0f);
	MinDistanceBetweenEnemyTargets = FMath::Max(MinDistanceBetweenEnemyTargets, 0.0f);
	FlankRadiusMin = FMath::Max(FlankRadiusMin, 0.0f);
	FlankRadiusMax = FMath::Max(FlankRadiusMax, FlankRadiusMin);
	TargetRandomOffsetMin = FMath::Max(TargetRandomOffsetMin, 0.0f);
	TargetRandomOffsetMax = FMath::Max(TargetRandomOffsetMax, TargetRandomOffsetMin);
	TargetReservationAttempts = FMath::Clamp(TargetReservationAttempts, 1, 20);
	CautiousFlankStartRadius = FMath::Max(CautiousFlankStartRadius, 0.0f);
	CautiousFlankMinRadius = FMath::Clamp(CautiousFlankMinRadius, 0.0f, CautiousFlankStartRadius);
	CautiousFlankApproachSpeedMin = FMath::Max(CautiousFlankApproachSpeedMin, 0.0f);
	CautiousFlankApproachSpeedMax = FMath::Max(CautiousFlankApproachSpeedMax, CautiousFlankApproachSpeedMin);
	RushFlankStartRadius = FMath::Max(RushFlankStartRadius, 0.0f);
	RushFlankMinRadius = FMath::Clamp(RushFlankMinRadius, 0.0f, RushFlankStartRadius);
	RushFlankApproachSpeedMin = FMath::Max(RushFlankApproachSpeedMin, 0.0f);
	RushFlankApproachSpeedMax = FMath::Max(RushFlankApproachSpeedMax, RushFlankApproachSpeedMin);
	FlankSideLockDurationMin = FMath::Max(FlankSideLockDurationMin, 0.0f);
	FlankSideLockDurationMax = FMath::Max(FlankSideLockDurationMax, FlankSideLockDurationMin);
	MaxAllowedFlankDistanceIncrease = FMath::Max(MaxAllowedFlankDistanceIncrease, 0.0f);
	FlankTargetLateralOffsetMax = FMath::Max(FlankTargetLateralOffsetMax, 0.0f);
	FlankClosePressureRange = FMath::Max(FlankClosePressureRange, 0.0f);
	ChaserRepathIntervalMin = FMath::Max(ChaserRepathIntervalMin, 0.05f);
	ChaserRepathIntervalMax = FMath::Max(ChaserRepathIntervalMax, ChaserRepathIntervalMin);
	FlankerRepathIntervalMin = FMath::Max(FlankerRepathIntervalMin, 0.05f);
	FlankerRepathIntervalMax = FMath::Max(FlankerRepathIntervalMax, FlankerRepathIntervalMin);
	BlockerRepathIntervalMin = FMath::Max(BlockerRepathIntervalMin, 0.05f);
	BlockerRepathIntervalMax = FMath::Max(BlockerRepathIntervalMax, BlockerRepathIntervalMin);
}

void AOBEnemy::RefreshAIStateFromBullet()
{
	const bool bHoldingBullet = IsPlayerHoldingBullet();
	if (bWasHoldingBullet == bHoldingBullet)
	{
		return;
	}

	bWasHoldingBullet = bHoldingBullet;
	if (bHoldingBullet)
	{
		bRushTransitionPending = false;
		GetWorldTimerManager().ClearTimer(RushTransitionTimerHandle);
		SetAIState(EOBEnemyAIState::Cautious);
	}
	else
	{
		ScheduleRushTransition();
	}
}

void AOBEnemy::ScheduleRushTransition()
{
	if (bRushTransitionPending || CurrentAIState == EOBEnemyAIState::Rush || bDead)
	{
		return;
	}

	bRushTransitionPending = true;
	const float Delay = FMath::FRandRange(RushTransitionDelayMin, RushTransitionDelayMax);
	UE_LOG(
		LogOBEnemyAI,
		Log,
		TEXT("%s scheduling Cautious -> Rush in %.2fs"),
		*GetName(),
		Delay);

	if (Delay <= KINDA_SMALL_NUMBER)
	{
		CompleteRushTransition();
		return;
	}

	GetWorldTimerManager().SetTimer(RushTransitionTimerHandle, this, &AOBEnemy::CompleteRushTransition, Delay, false);
}

void AOBEnemy::CompleteRushTransition()
{
	bRushTransitionPending = false;
	if (!bDead && !IsPlayerHoldingBullet())
	{
		SetAIState(EOBEnemyAIState::Rush);
	}
}

void AOBEnemy::SetAIState(EOBEnemyAIState NewState, bool bForceRefresh)
{
	if (!bForceRefresh && CurrentAIState == NewState)
	{
		return;
	}

	const EOBEnemyAIState PreviousState = CurrentAIState;
	CurrentAIState = NewState;
	CurrentStateElapsed = 0.0f;
	bRushTransitionPending = false;
	GetWorldTimerManager().ClearTimer(RushTransitionTimerHandle);
	AssignEnemyRoles();

	const float SpeedMin = CurrentAIState == EOBEnemyAIState::Cautious
		? CautiousSpeedMultiplierMin
		: RushSpeedMultiplierMin;
	const float SpeedMax = CurrentAIState == EOBEnemyAIState::Cautious
		? CautiousSpeedMultiplierMax
		: RushSpeedMultiplierMax;
	const float Variance = CurrentAIState == EOBEnemyAIState::Cautious
		? CautiousSpeedRandomVariance
		: RushSpeedRandomVariance;
	const float BaseMultiplier = FMath::FRandRange(SpeedMin, SpeedMax);
	CurrentStateSpeedMultiplier = BaseMultiplier * FMath::FRandRange(1.0f - Variance, 1.0f + Variance);

	ApplyAIStateSpeed();
	ActiveLocomotionAnimation = nullptr;
	ActiveLocomotionPlayRate = -1.0f;
	ResetMovementForStateChange();

	UE_LOG(
		LogOBEnemyAI,
		Log,
		TEXT("%s AI state %s -> %s, role=%s, speed=%.2fx"),
		*GetName(),
		GetAIStateName(PreviousState),
		GetAIStateName(CurrentAIState),
		GetAIRoleName(CurrentRole),
		CurrentStateSpeedMultiplier);
	OnEnemyAIStateChanged(CurrentAIState, CurrentRole);
}

void AOBEnemy::GetLiveEnemies(TArray<AOBEnemy*>& OutEnemies) const
{
	OutEnemies.Reset();
	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(this, AOBEnemy::StaticClass(), EnemyActors);
	for (AActor* Actor : EnemyActors)
	{
		AOBEnemy* Enemy = Cast<AOBEnemy>(Actor);
		if (Enemy && !Enemy->IsDead())
		{
			OutEnemies.Add(Enemy);
		}
	}

	OutEnemies.Sort([](const AOBEnemy& Left, const AOBEnemy& Right)
	{
		return Left.GetUniqueID() < Right.GetUniqueID();
	});
}

void AOBEnemy::AssignEnemyRoles()
{
	TArray<AOBEnemy*> LiveEnemies;
	GetLiveEnemies(LiveEnemies);
	if (LiveEnemies.IsEmpty())
	{
		return;
	}

	auto AssignStateGroup = [this](const TArray<AOBEnemy*>& Group, EOBEnemyAIState State)
	{
		const int32 EnemyCount = Group.Num();
		if (EnemyCount == 0)
		{
			return;
		}

		const bool bCanUseBlocker = State == EOBEnemyAIState::Rush && FindActiveBulletPickup() != nullptr;
		int32 ChaserCount = 1;
		int32 BlockerCount = 0;
		if (EnemyCount >= 3)
		{
			const float ChaserChance = State == EOBEnemyAIState::Cautious ? CautiousChaserChance : RushChaserChance;
			const float FlankerChance = State == EOBEnemyAIState::Cautious ? CautiousFlankerChance : RushFlankerChance;
			const float BlockerChance = bCanUseBlocker ? RushBulletBlockerChance : 0.0f;
			const float TotalChance = FMath::Max(ChaserChance + FlankerChance + BlockerChance, KINDA_SMALL_NUMBER);
			ChaserCount = FMath::Clamp(FMath::RoundToInt(EnemyCount * ChaserChance / TotalChance), 1, EnemyCount);
			BlockerCount = FMath::Clamp(
				FMath::RoundToInt(EnemyCount * BlockerChance / TotalChance),
				0,
				EnemyCount - ChaserCount);
		}
		else if (EnemyCount == 2 && bCanUseBlocker && RushBulletBlockerChance >= RushFlankerChance)
		{
			BlockerCount = 1;
		}

		const int32 FlankerCount = EnemyCount - ChaserCount - BlockerCount;
		int32 FlankerSlot = 0;
		int32 BlockerSlot = 0;
		for (int32 Index = 0; Index < EnemyCount; ++Index)
		{
			EOBEnemyRole AssignedRole = EOBEnemyRole::Flanker;
			int32 RoleSlot = FlankerSlot++;
			int32 RoleCount = FlankerCount;
			if (Index < ChaserCount)
			{
				AssignedRole = EOBEnemyRole::Chaser;
				RoleSlot = Index;
				RoleCount = ChaserCount;
			}
			else if (Index >= ChaserCount + FlankerCount)
			{
				AssignedRole = EOBEnemyRole::BulletBlocker;
				RoleSlot = BlockerSlot++;
				RoleCount = BlockerCount;
			}
			Group[Index]->SetAssignedRole(AssignedRole, RoleSlot, RoleCount);
		}
	};

	TArray<AOBEnemy*> CautiousEnemies;
	TArray<AOBEnemy*> RushEnemies;
	for (AOBEnemy* Enemy : LiveEnemies)
	{
		(Enemy->CurrentAIState == EOBEnemyAIState::Cautious ? CautiousEnemies : RushEnemies).Add(Enemy);
	}
	AssignStateGroup(CautiousEnemies, EOBEnemyAIState::Cautious);
	AssignStateGroup(RushEnemies, EOBEnemyAIState::Rush);
}

void AOBEnemy::SetAssignedRole(EOBEnemyRole NewRole, int32 RoleSlot, int32 RoleCount)
{
	const EOBEnemyRole PreviousRole = CurrentRole;
	CurrentRole = NewRole;
	if (CurrentRole == EOBEnemyRole::Flanker)
	{
		const bool bNeedsNewFlankSetup = !bHasFlankSetup
			|| PreviousRole != EOBEnemyRole::Flanker
			|| FlankerRadiusState != CurrentAIState;
		if (bNeedsNewFlankSetup)
		{
			static constexpr float FlankSectorAngles[] = {-65.0f, 65.0f, -125.0f, 125.0f};
			const int32 SectorIndex = RoleCount <= 1
				? static_cast<int32>(GetUniqueID() % UE_ARRAY_COUNT(FlankSectorAngles))
				: RoleSlot % UE_ARRAY_COUNT(FlankSectorAngles);
			FlankerSlotAngleDegrees = FlankSectorAngles[SectorIndex];
			LockedFlankDirection = GetPlayerMovementDirection()
				.RotateAngleAxis(FlankerSlotAngleDegrees, FVector::UpVector)
				.GetSafeNormal2D();
			LockedFlankLateralOffset = FMath::FRandRange(
				-FlankTargetLateralOffsetMax,
				FlankTargetLateralOffsetMax);
			FlankSideLockRemaining = FMath::FRandRange(FlankSideLockDurationMin, FlankSideLockDurationMax);
			bHasFlankSetup = true;
			FlankerRadiusState = CurrentAIState;
			CurrentFlankerOffsetDistance = CurrentAIState == EOBEnemyAIState::Cautious
				? CautiousFlankStartRadius
				: RushFlankStartRadius;
			CurrentFlankApproachSpeed = CurrentAIState == EOBEnemyAIState::Cautious
				? FMath::FRandRange(CautiousFlankApproachSpeedMin, CautiousFlankApproachSpeedMax)
				: FMath::FRandRange(RushFlankApproachSpeedMin, RushFlankApproachSpeedMax);
		}
	}

	if (PreviousRole != CurrentRole)
	{
		UE_LOG(
			LogOBEnemyAI,
			Log,
			TEXT("%s role %s -> %s (%s)"),
			*GetName(),
			GetAIRoleName(PreviousRole),
			GetAIRoleName(CurrentRole),
			GetAIStateName(CurrentAIState));
		OnEnemyAIStateChanged(CurrentAIState, CurrentRole);
		if (HasActorBegunPlay() && !bDead)
		{
			ResetMovementForStateChange();
		}
	}
}

void AOBEnemy::ApplyAIStateSpeed()
{
	const float BaseSpeed = EnemyType == EOBEnemyType::Heavy ? HeavySpeed : FastSpeed;
	GetCharacterMovement()->MaxWalkSpeed =
		BaseSpeed
		* FMath::Max(CurrentStateSpeedMultiplier, 0.0f)
		* FMath::Max(DifficultySpeedMultiplier, 0.0f);
}

void AOBEnemy::ResetMovementForStateChange()
{
	bUsingDirectMovementFallback = false;
	bMovingToPatrolTarget = false;
	bHasPatrolTarget = false;
	CurrentPatrolPath.Reset();
	CurrentPatrolPathIndex = 0;
	CurrentApproachTarget = FVector::ZeroVector;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}

	if (HasActorBegunPlay() && !bDead && !bStunned)
	{
		RequestMove();
	}
}

FVector AOBEnemy::CalculateStateMovementTarget()
{
	switch (CurrentRole)
	{
	case EOBEnemyRole::Chaser:
		return CalculateChaserTarget();
	case EOBEnemyRole::Flanker:
		return CalculateFlankerTarget(CurrentAIState == EOBEnemyAIState::Cautious);
	case EOBEnemyRole::BulletBlocker:
		return CalculateBulletBlockerTarget();
	default:
		return GetActorLocation();
	}
}

FVector AOBEnemy::CalculateChaserTarget() const
{
	return PlayerTarget ? PlayerTarget->GetActorLocation() : GetActorLocation();
}

FVector AOBEnemy::CalculateFlankerTarget(bool bCompressDistance)
{
	if (!PlayerTarget)
	{
		return GetActorLocation();
	}
	(void)bCompressDistance;

	const FVector PlayerLocation = PlayerTarget->GetActorLocation();
	const float CurrentDistanceToPlayer = FVector::Dist2D(GetActorLocation(), PlayerLocation);
	const float MinFlankRadius = CurrentAIState == EOBEnemyAIState::Cautious
		? CautiousFlankMinRadius
		: RushFlankMinRadius;
	if (CurrentDistanceToPlayer <= MinFlankRadius + FlankClosePressureRange)
	{
		FVector PressureTarget = PlayerLocation;
		ProjectPointToNavigation(PressureTarget);
		return PressureTarget;
	}

	if (FlankSideLockRemaining <= 0.0f)
	{
		LockedFlankDirection = GetPlayerMovementDirection()
			.RotateAngleAxis(FlankerSlotAngleDegrees, FVector::UpVector)
			.GetSafeNormal2D();
		LockedFlankLateralOffset = FMath::FRandRange(
			-FlankTargetLateralOffsetMax,
			FlankTargetLateralOffsetMax);
		FlankSideLockRemaining = FMath::FRandRange(FlankSideLockDurationMin, FlankSideLockDurationMax);
	}

	const float MaxTargetRadius = CurrentDistanceToPlayer + MaxAllowedFlankDistanceIncrease;
	const float TargetRadius = FMath::Min(CurrentFlankerOffsetDistance, MaxTargetRadius);
	const FVector SideDirection = LockedFlankDirection.RotateAngleAxis(90.0f, FVector::UpVector);
	FVector FallbackTarget = PlayerLocation
		+ LockedFlankDirection * TargetRadius
		+ SideDirection * LockedFlankLateralOffset;
	if (FVector::DistSquared2D(FallbackTarget, PlayerLocation) > FMath::Square(MaxTargetRadius))
	{
		FallbackTarget = PlayerLocation
			+ (FallbackTarget - PlayerLocation).GetSafeNormal2D() * MaxTargetRadius;
	}
	ProjectPointToNavigation(FallbackTarget);
	if (FVector::DistSquared2D(FallbackTarget, PlayerLocation) > FMath::Square(MaxTargetRadius))
	{
		FallbackTarget = PlayerLocation
			+ (FallbackTarget - PlayerLocation).GetSafeNormal2D() * MaxTargetRadius;
		ProjectPointToNavigation(FallbackTarget);
	}

	return FindUnclaimedTarget(
		[this, PlayerLocation, SideDirection, TargetRadius, MaxTargetRadius](int32 Attempt)
		{
			const float AttemptOffset = Attempt == 0
				? 0.0f
				: FMath::FRandRange(-FlankTargetLateralOffsetMax * 0.35f, FlankTargetLateralOffsetMax * 0.35f);
			FVector Candidate = PlayerLocation
				+ LockedFlankDirection * TargetRadius
				+ SideDirection * (LockedFlankLateralOffset + AttemptOffset);
			const FVector PlayerToCandidate = (Candidate - PlayerLocation).GetSafeNormal2D();
			const float CandidateRadius = FVector::Dist2D(Candidate, PlayerLocation);
			if (CandidateRadius > MaxTargetRadius)
			{
				Candidate = PlayerLocation + PlayerToCandidate * MaxTargetRadius;
			}
			ProjectPointToNavigation(Candidate);
			if (FVector::DistSquared2D(Candidate, PlayerLocation) > FMath::Square(MaxTargetRadius))
			{
				Candidate = PlayerLocation
					+ (Candidate - PlayerLocation).GetSafeNormal2D() * MaxTargetRadius;
				ProjectPointToNavigation(Candidate);
			}
			return Candidate;
		},
		FallbackTarget);
}

FVector AOBEnemy::CalculateBulletBlockerTarget()
{
	if (!PlayerTarget)
	{
		return GetActorLocation();
	}

	if (const AOBBulletPickup* BulletPickup = FindActiveBulletPickup())
	{
		const FVector PlayerLocation = PlayerTarget->GetActorLocation();
		const FVector BulletLocation = BulletPickup->GetActorLocation();
		const FVector PlayerToBullet = (BulletLocation - PlayerLocation).GetSafeNormal2D();
		const FVector SideDirection = PlayerToBullet.RotateAngleAxis(90.0f, FVector::UpVector);
		FVector FallbackTarget = FMath::Lerp(PlayerLocation, BulletLocation, 0.5f);
		ProjectPointToNavigation(FallbackTarget);

		return FindUnclaimedTarget(
			[this, PlayerLocation, BulletLocation, SideDirection](int32 Attempt)
			{
				const float MidpointAlpha = FMath::Clamp(0.5f + FMath::FRandRange(-0.12f, 0.12f), 0.25f, 0.75f);
				const float SideSign = ((GetUniqueID() + Attempt) & 1) == 0 ? 1.0f : -1.0f;
				const float SideOffset = FMath::FRandRange(TargetRandomOffsetMin, TargetRandomOffsetMax) * SideSign;
				FVector Candidate = FMath::Lerp(PlayerLocation, BulletLocation, MidpointAlpha) + SideDirection * SideOffset;
				ProjectPointToNavigation(Candidate);
				return Candidate;
			},
			FallbackTarget);
	}

	return CalculateFlankerTarget(false);
}

FVector AOBEnemy::FindUnclaimedTarget(const TFunction<FVector(int32)>& CandidateGenerator, const FVector& FallbackTarget)
{
	FVector BestCandidate = FallbackTarget;
	float BestSeparationSquared = -1.0f;
	for (int32 Attempt = 0; Attempt < TargetReservationAttempts; ++Attempt)
	{
		const FVector Candidate = CandidateGenerator(Attempt);
		if (!IsTargetClaimedByAnotherEnemy(Candidate))
		{
			return Candidate;
		}

		float ClosestClaimSquared = TNumericLimits<float>::Max();
		TArray<AOBEnemy*> LiveEnemies;
		GetLiveEnemies(LiveEnemies);
		for (const AOBEnemy* Enemy : LiveEnemies)
		{
			if (Enemy != this && !Enemy->CurrentApproachTarget.IsNearlyZero())
			{
				ClosestClaimSquared = FMath::Min(
					ClosestClaimSquared,
					FVector::DistSquared2D(Candidate, Enemy->CurrentApproachTarget));
			}
		}
		if (ClosestClaimSquared > BestSeparationSquared)
		{
			BestSeparationSquared = ClosestClaimSquared;
			BestCandidate = Candidate;
		}
	}

	return BestCandidate;
}

bool AOBEnemy::IsTargetClaimedByAnotherEnemy(const FVector& Candidate) const
{
	if (MinDistanceBetweenEnemyTargets <= 0.0f)
	{
		return false;
	}

	TArray<AOBEnemy*> LiveEnemies;
	GetLiveEnemies(LiveEnemies);
	for (const AOBEnemy* Enemy : LiveEnemies)
	{
		if (Enemy == this || Enemy->CurrentApproachTarget.IsNearlyZero())
		{
			continue;
		}
		if (FVector::DistSquared2D(Candidate, Enemy->CurrentApproachTarget) < FMath::Square(MinDistanceBetweenEnemyTargets))
		{
			return true;
		}
	}
	return false;
}

FVector AOBEnemy::GetPlayerMovementDirection() const
{
	if (!PlayerTarget)
	{
		return FVector::ForwardVector;
	}

	FVector MovementDirection = PlayerTarget->GetVelocity().GetSafeNormal2D();
	if (!MovementDirection.IsNearlyZero())
	{
		return MovementDirection;
	}

	if (const AController* PlayerController = PlayerTarget->GetController())
	{
		MovementDirection = FRotationMatrix(FRotator(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X).GetSafeNormal2D();
	}

	return MovementDirection.IsNearlyZero()
		? PlayerTarget->GetActorForwardVector().GetSafeNormal2D()
		: MovementDirection;
}

AOBBulletPickup* AOBEnemy::FindActiveBulletPickup() const
{
	TArray<AActor*> PickupActors;
	UGameplayStatics::GetAllActorsOfClass(this, AOBBulletPickup::StaticClass(), PickupActors);

	AOBBulletPickup* ClosestPickup = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	const FVector ReferenceLocation = PlayerTarget ? PlayerTarget->GetActorLocation() : GetActorLocation();
	for (AActor* Actor : PickupActors)
	{
		AOBBulletPickup* Pickup = Cast<AOBBulletPickup>(Actor);
		if (!IsValid(Pickup))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(ReferenceLocation, Pickup->GetActorLocation());
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestPickup = Pickup;
		}
	}
	return ClosestPickup;
}

const TCHAR* AOBEnemy::GetAIStateName(EOBEnemyAIState State) const
{
	switch (State)
	{
	case EOBEnemyAIState::Cautious:
		return TEXT("Cautious");
	case EOBEnemyAIState::Rush:
		return TEXT("Rush");
	default:
		return TEXT("Unknown");
	}
}

const TCHAR* AOBEnemy::GetAIRoleName(EOBEnemyRole AIRole) const
{
	switch (AIRole)
	{
	case EOBEnemyRole::Chaser:
		return TEXT("Chaser");
	case EOBEnemyRole::Flanker:
		return TEXT("Flanker");
	case EOBEnemyRole::BulletBlocker:
		return TEXT("BulletBlocker");
	default:
		return TEXT("Unknown");
	}
}

bool AOBEnemy::IsPlayerHoldingBullet() const
{
	const AOBGameState* OneBulletState = GetWorld() ? GetWorld()->GetGameState<AOBGameState>() : nullptr;
	return !OneBulletState || OneBulletState->HasBullet();
}

bool AOBEnemy::IsPlayerInsideBulletAttackRadius() const
{
	return PlayerTarget
		&& FVector::DistSquared2D(GetActorLocation(), PlayerTarget->GetActorLocation()) <= FMath::Square(PlayerHasBulletAttackRadius);
}

bool AOBEnemy::IsPlayerDetected() const
{
	return PlayerTarget
		&& DetectionRadius > 0.0f
		&& FVector::DistSquared2D(GetActorLocation(), PlayerTarget->GetActorLocation()) <= FMath::Square(DetectionRadius);
}

bool AOBEnemy::ShouldPatrolWhilePlayerHasBullet() const
{
	if (!PlayerTarget || !IsPlayerHoldingBullet())
	{
		return false;
	}

	return !IsPlayerDetected();
}

FVector AOBEnemy::GetOrChoosePatrolTarget()
{
	if (bHasPatrolTarget && FVector::DistSquared2D(GetActorLocation(), CurrentPatrolTarget) > FMath::Square(PatrolAcceptanceRadius))
	{
		return CurrentPatrolTarget;
	}

	CurrentPatrolTarget = ChooseWholeArenaPatrolTarget();
	bHasPatrolTarget = true;
	CurrentPatrolPath.Reset();
	CurrentPatrolPathIndex = 0;
	LastPatrolLocation = GetActorLocation();
	PatrolStuckTime = 0.0f;
	return CurrentPatrolTarget;
}

FVector AOBEnemy::ChooseWholeArenaPatrolTarget() const
{
	const float EffectivePatrolRadius = FMath::Max(PatrolRadius, PatrolAcceptanceRadius * 2.0f);
	const float MinTargetDistance = FMath::Clamp(PatrolMinTargetDistance, PatrolAcceptanceRadius, EffectivePatrolRadius * 0.75f);
	FVector FromCenter = GetActorLocation() - PatrolOrigin;
	FromCenter.Z = 0.0f;
	if (FromCenter.IsNearlyZero())
	{
		const float SeedAngle = FMath::Fmod(static_cast<float>(GetUniqueID() % 360), 360.0f);
		FromCenter = FVector(FMath::Cos(FMath::DegreesToRadians(SeedAngle)), FMath::Sin(FMath::DegreesToRadians(SeedAngle)), 0.0f);
	}

	const float BaseAngle = FMath::RadiansToDegrees(FMath::Atan2(FromCenter.Y, FromCenter.X));
	const int32 DirectionSign = (GetUniqueID() % 2 == 0) ? 1 : -1;
	const TArray<float> RadiusMultipliers =
	{
		PatrolPerimeterRadiusMultiplier,
		PatrolPerimeterRadiusMultiplier * 0.85f,
		PatrolPerimeterRadiusMultiplier * 0.70f
	};

	for (const float RadiusMultiplier : RadiusMultipliers)
	{
		const float PerimeterRadius = EffectivePatrolRadius * FMath::Clamp(RadiusMultiplier, 0.1f, 1.0f);
		for (int32 Attempt = 1; Attempt <= 16; ++Attempt)
		{
			const float CandidateAngle = BaseAngle + DirectionSign * PatrolPerimeterStepDegrees * static_cast<float>(Attempt);
			const FVector Direction(
				FMath::Cos(FMath::DegreesToRadians(CandidateAngle)),
				FMath::Sin(FMath::DegreesToRadians(CandidateAngle)),
				0.0f);
			const FVector Jitter = Direction.RotateAngleAxis(90.0f, FVector::UpVector) * FMath::FRandRange(-PatrolPointJitter, PatrolPointJitter);
			FVector Candidate = PatrolOrigin + Direction * PerimeterRadius + Jitter;
			Candidate.Z = GetActorLocation().Z;

			if (FVector::DistSquared2D(GetActorLocation(), Candidate) <= FMath::Square(MinTargetDistance))
			{
				continue;
			}

			if (ProjectPointToNavigation(Candidate) && IsPatrolCandidateClear(Candidate))
			{
				return Candidate;
			}
		}
	}

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		for (int32 Attempt = 0; Attempt < 32; ++Attempt)
		{
			FNavLocation RandomLocation;
			if (NavSystem->GetRandomReachablePointInRadius(GetActorLocation(), EffectivePatrolRadius * 0.55f, RandomLocation)
				&& FVector::DistSquared2D(GetActorLocation(), RandomLocation.Location) >= FMath::Square(MinTargetDistance)
				&& IsPatrolCandidateClear(RandomLocation.Location))
			{
				return RandomLocation.Location;
			}
		}

		for (int32 Attempt = 0; Attempt < 32; ++Attempt)
		{
			FNavLocation RandomLocation;
			if (NavSystem->GetRandomReachablePointInRadius(PatrolOrigin, EffectivePatrolRadius * 0.80f, RandomLocation)
				&& FVector::DistSquared2D(GetActorLocation(), RandomLocation.Location) >= FMath::Square(PatrolAcceptanceRadius * 2.0f)
				&& IsPatrolCandidateClear(RandomLocation.Location))
			{
				return RandomLocation.Location;
			}
		}
	}

	return GetActorLocation();
}

bool AOBEnemy::ProjectPointToNavigation(FVector& InOutLocation) const
{
	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavSystem->ProjectPointToNavigation(InOutLocation, ProjectedLocation, FVector(320.0f, 320.0f, 500.0f)))
		{
			InOutLocation = ProjectedLocation.Location;
			return true;
		}
	}

	return false;
}

bool AOBEnemy::IsPatrolCandidateClear(const FVector& Candidate) const
{
	if (!GetWorld() || FVector::DistSquared2D(GetActorLocation(), Candidate) <= FMath::Square(PatrolAcceptanceRadius))
	{
		return false;
	}

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		UNavigationPath* Path = NavSystem->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), Candidate, const_cast<AOBEnemy*>(this));
		return Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num() > 1;
	}

	return false;
}

bool AOBEnemy::TryGetPatrolSteeringTarget(FVector& OutTarget) const
{
	OutTarget = CurrentPatrolTarget;
	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		UNavigationPath* Path = NavSystem->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), CurrentPatrolTarget, const_cast<AOBEnemy*>(this));
		if (!Path || !Path->IsValid() || Path->PathPoints.Num() < 2)
		{
			return true;
		}

		for (int32 PointIndex = 1; PointIndex < Path->PathPoints.Num(); ++PointIndex)
		{
			if (FVector::DistSquared2D(GetActorLocation(), Path->PathPoints[PointIndex]) > FMath::Square(90.0f))
			{
				OutTarget = Path->PathPoints[PointIndex];
				return true;
			}
		}

		OutTarget = CurrentPatrolTarget;
		return true;
	}

	return true;
}

FVector AOBEnemy::CalculatePatrolMovementDirection(const FVector& DesiredDirection) const
{
	if (DesiredDirection.IsNearlyZero() || !GetWorld() || PatrolObstacleProbeDistance <= 0.0f || PatrolObstacleProbeRadius <= 0.0f)
	{
		return DesiredDirection;
	}

	const FVector Origin = GetActorLocation();
	const FVector TraceStart = Origin + FVector(0.0f, 0.0f, 45.0f);
	const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(PatrolObstacleProbeRadius);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyPatrolObstacleProbe), false, this);

	auto IsDirectionBlocked = [&](const FVector& Direction)
	{
		FHitResult Hit;
		const FVector TraceEnd = TraceStart + Direction.GetSafeNormal2D() * PatrolObstacleProbeDistance;
		return GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, ECC_WorldStatic, ProbeShape, QueryParams);
	};

	if (!IsDirectionBlocked(DesiredDirection))
	{
		return DesiredDirection;
	}

	const FVector LeftDirection = DesiredDirection.RotateAngleAxis(-70.0f, FVector::UpVector).GetSafeNormal2D();
	const FVector RightDirection = DesiredDirection.RotateAngleAxis(70.0f, FVector::UpVector).GetSafeNormal2D();
	const bool bLeftBlocked = IsDirectionBlocked(LeftDirection);
	const bool bRightBlocked = IsDirectionBlocked(RightDirection);
	if (!bLeftBlocked)
	{
		return LeftDirection;
	}
	if (!bRightBlocked)
	{
		return RightDirection;
	}

	return FVector::ZeroVector;
}

bool AOBEnemy::MoveToCurrentTarget(float AcceptanceRadius, bool bAllowDirectFallback, bool bAllowPartialPath)
{
	bUsingDirectMovementFallback = false;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		const EPathFollowingRequestResult::Type MoveResult = AI->MoveToLocation(CurrentApproachTarget, AcceptanceRadius, true, true, true, true, nullptr, bAllowPartialPath);
		if (MoveResult != EPathFollowingRequestResult::Failed)
		{
			return true;
		}

		bUsingDirectMovementFallback = bAllowDirectFallback && bUseDirectMovementFallback;
		return bUsingDirectMovementFallback;
	}

	bUsingDirectMovementFallback = bAllowDirectFallback && bUseDirectMovementFallback;
	return bUsingDirectMovementFallback;
}

bool AOBEnemy::RebuildPatrolPath()
{
	CurrentPatrolPath.Reset();
	CurrentPatrolPathIndex = 0;

	if (!bHasPatrolTarget || !GetWorld())
	{
		return false;
	}

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		UNavigationPath* Path = NavSystem->FindPathToLocationSynchronously(GetWorld(), GetActorLocation(), CurrentPatrolTarget, this);
		if (Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num() > 1)
		{
			CurrentPatrolPath = Path->PathPoints;
			CurrentPatrolPathIndex = 1;
			return true;
		}
	}

	return false;
}

void AOBEnemy::UpdatePatrolMovement(float DeltaSeconds)
{
	if (!bHasPatrolTarget)
	{
		ChooseNewPatrolTarget();
		return;
	}

	const float DistanceToTargetSquared = FVector::DistSquared2D(GetActorLocation(), CurrentPatrolTarget);
	if (DistanceToTargetSquared <= FMath::Square(PatrolAcceptanceRadius))
	{
		ChooseNewPatrolTarget();
		return;
	}

	const float MovedDistanceSquared = FVector::DistSquared2D(GetActorLocation(), LastPatrolLocation);
	if (MovedDistanceSquared <= FMath::Square(12.0f))
	{
		PatrolStuckTime += DeltaSeconds;
		if (PatrolStuckTime >= 0.45f)
		{
			if (AAIController* AI = Cast<AAIController>(GetController()))
			{
				AI->StopMovement();
			}
			ChooseNewPatrolTarget();
			return;
		}
	}
	else
	{
		LastPatrolLocation = GetActorLocation();
		PatrolStuckTime = 0.0f;
	}

	if (CurrentPatrolPath.Num() < 2 || CurrentPatrolPathIndex <= 0 || CurrentPatrolPathIndex >= CurrentPatrolPath.Num())
	{
		if (!RebuildPatrolPath())
		{
			ChooseNewPatrolTarget();
			return;
		}
	}

	while (CurrentPatrolPathIndex < CurrentPatrolPath.Num()
		&& FVector::DistSquared2D(GetActorLocation(), CurrentPatrolPath[CurrentPatrolPathIndex]) <= FMath::Square(80.0f))
	{
		++CurrentPatrolPathIndex;
	}

	if (CurrentPatrolPathIndex >= CurrentPatrolPath.Num())
	{
		ChooseNewPatrolTarget();
		return;
	}

	CurrentApproachTarget = CurrentPatrolPath[CurrentPatrolPathIndex];
	const FVector MovementDirection = (CurrentApproachTarget - GetActorLocation()).GetSafeNormal2D();
	if (MovementDirection.IsNearlyZero())
	{
		ChooseNewPatrolTarget();
		return;
	}

	AddMovementInput(MovementDirection, 1.0f, true);
}

void AOBEnemy::ChooseNewPatrolTarget()
{
	bHasPatrolTarget = false;
	CurrentPatrolPath.Reset();
	CurrentPatrolPathIndex = 0;
	CurrentApproachTarget = GetOrChoosePatrolTarget();
	if (FVector::DistSquared2D(GetActorLocation(), CurrentApproachTarget) <= FMath::Square(PatrolAcceptanceRadius))
	{
		bHasPatrolTarget = false;
		CurrentPatrolPath.Reset();
		CurrentPatrolPathIndex = 0;
		CurrentApproachTarget = GetOrChoosePatrolTarget();
	}
	if (!RebuildPatrolPath())
	{
		bHasPatrolTarget = false;
		CurrentPatrolPath.Reset();
		CurrentPatrolPathIndex = 0;
	}
}

void AOBEnemy::MoveAggressivelyToPlayer()
{
	bMovingToPatrolTarget = false;
	bHasPatrolTarget = false;
	CurrentApproachTarget = PlayerTarget ? PlayerTarget->GetActorLocation() : GetActorLocation();

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->MoveToActor(PlayerTarget, 55.0f, true, true, true, nullptr, true);
	}
	else
	{
		bUsingDirectMovementFallback = bUseDirectMovementFallback;
		CurrentApproachTarget = PlayerTarget ? PlayerTarget->GetActorLocation() : CurrentApproachTarget;
	}
}

void AOBEnemy::ApplyDirectLostBulletChase()
{
	if (!PlayerTarget)
	{
		return;
	}

	const FVector DirectionToPlayer = (PlayerTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!DirectionToPlayer.IsNearlyZero())
	{
		AddMovementInput(DirectionToPlayer, 1.0f, true);
	}
}

void AOBEnemy::UpdateSimpleLocomotionAnimation()
{
	if (!bUseSimpleLocomotionAnimations || bDead || !GetMesh())
	{
		return;
	}

	const float SpeedSquared = GetVelocity().SizeSquared2D();
	UAnimationAsset* DesiredAnimation = IdleAnimation.Get();
	float DesiredPlayRate = 1.0f;
	if (SpeedSquared >= FMath::Square(WalkAnimationMinSpeed))
	{
		if (CurrentAIState == EOBEnemyAIState::Cautious)
		{
			DesiredAnimation = WalkAnimation ? WalkAnimation.Get() : IdleAnimation.Get();
			DesiredPlayRate = WalkAnimation ? WalkAnimationPlayRate : 1.0f;
		}
		else
		{
			DesiredAnimation = RunAnimation ? RunAnimation.Get() : IdleAnimation.Get();
			DesiredPlayRate = RunAnimation ? RunAnimationPlayRate : 1.0f;
		}
	}

	if (DesiredAnimation && !IsAnimationCompatibleWithMesh(DesiredAnimation, GetMesh()))
	{
		DesiredAnimation = IdleAnimation.Get();
		DesiredPlayRate = 1.0f;
	}
	if (!DesiredAnimation)
	{
		return;
	}

	const bool bAnimationChanged = DesiredAnimation != ActiveLocomotionAnimation;
	const bool bPlayRateChanged = !FMath::IsNearlyEqual(DesiredPlayRate, ActiveLocomotionPlayRate, 0.01f);
	if (!bAnimationChanged && !bPlayRateChanged)
	{
		return;
	}

	if (bAnimationChanged)
	{
		ActiveLocomotionAnimation = DesiredAnimation;
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		GetMesh()->PlayAnimation(DesiredAnimation, true);
	}
	ActiveLocomotionPlayRate = DesiredPlayRate;
	if (UAnimSingleNodeInstance* SingleNodeInstance = GetMesh()->GetSingleNodeInstance())
	{
		SingleNodeInstance->SetPlayRate(DesiredPlayRate);
	}
}

float AOBEnemy::GetNextRepathInterval() const
{
	float IntervalMin = ChaserRepathIntervalMin;
	float IntervalMax = ChaserRepathIntervalMax;
	if (CurrentRole == EOBEnemyRole::Flanker)
	{
		IntervalMin = FlankerRepathIntervalMin;
		IntervalMax = FlankerRepathIntervalMax;
	}
	else if (CurrentRole == EOBEnemyRole::BulletBlocker)
	{
		IntervalMin = BlockerRepathIntervalMin;
		IntervalMax = BlockerRepathIntervalMax;
	}
	return FMath::FRandRange(IntervalMin, IntervalMax) * RepathTimeJitter;
}

void AOBEnemy::ScheduleNextMoveRequest(float InitialDelay)
{
	if (bDead || bSpawnWarningActive || !GetWorld())
	{
		return;
	}

	const float Delay = InitialDelay >= 0.0f ? InitialDelay : GetNextRepathInterval();
	GetWorldTimerManager().SetTimer(
		MoveTimerHandle,
		this,
		&AOBEnemy::RequestMove,
		FMath::Max(Delay, 0.01f),
		false);
}

void AOBEnemy::RequestMove()
{
	if (bDead || bSpawnWarningActive)
	{
		return;
	}
	ScheduleNextMoveRequest();
	if (bStunned)
	{
		return;
	}

	if (!PlayerTarget)
	{
		PlayerTarget = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	if (PlayerTarget && !PlayerTarget->IsDead())
	{
		bMovingToPatrolTarget = false;
		bHasPatrolTarget = false;
		CurrentApproachTarget = CalculateStateMovementTarget();

		if (CurrentRole == EOBEnemyRole::Chaser)
		{
			if (AAIController* AI = Cast<AAIController>(GetController()))
			{
				bUsingDirectMovementFallback = false;
				AI->MoveToActor(PlayerTarget, 20.0f, true, true, true, nullptr, true);
			}
			else
			{
				bUsingDirectMovementFallback = bUseDirectMovementFallback;
			}
			return;
		}

		float AcceptanceRadius = 90.0f;
		if (CurrentAIState == EOBEnemyAIState::Cautious)
		{
			AcceptanceRadius = 25.0f;
		}
		else if (CurrentRole == EOBEnemyRole::BulletBlocker)
		{
			AcceptanceRadius = BulletBlockerAcceptanceRadius;
		}
		else if (CurrentRole == EOBEnemyRole::Flanker)
		{
			AcceptanceRadius = 100.0f;
		}
		else
		{
			AcceptanceRadius = 55.0f;
		}

		MoveToCurrentTarget(AcceptanceRadius, true, true);
	}
}

FVector AOBEnemy::CalculateApproachTarget() const
{
	if (!PlayerTarget || !bUseSurroundMovement)
	{
		return PlayerTarget ? PlayerTarget->GetActorLocation() : GetActorLocation();
	}

	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(this, AOBEnemy::StaticClass(), EnemyActors);
	TArray<AOBEnemy*> LiveEnemies;
	for (AActor* Actor : EnemyActors)
	{
		AOBEnemy* Enemy = Cast<AOBEnemy>(Actor);
		if (Enemy && !Enemy->IsDead())
		{
			LiveEnemies.Add(Enemy);
		}
	}

	LiveEnemies.Sort([](const AOBEnemy& Left, const AOBEnemy& Right)
	{
		return Left.GetUniqueID() < Right.GetUniqueID();
	});

	const int32 MySlot = LiveEnemies.IndexOfByKey(const_cast<AOBEnemy*>(this));
	if (LiveEnemies.Num() <= 1 || MySlot == INDEX_NONE)
	{
		return PlayerTarget->GetActorLocation();
	}

	FVector PlayerForward = PlayerTarget->GetActorForwardVector().GetSafeNormal2D();
	if (const AController* PlayerController = PlayerTarget->GetController())
	{
		PlayerForward = FRotationMatrix(FRotator(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X).GetSafeNormal2D();
	}

	const float SlotAlpha = static_cast<float>(MySlot) / static_cast<float>(LiveEnemies.Num() - 1);
	const float YawOffset = FMath::Lerp(-SurroundFrontArcDegrees * 0.5f, SurroundFrontArcDegrees * 0.5f, SlotAlpha);
	const float TargetRadius = FMath::Max(
		EnemyType == EOBEnemyType::Heavy ? HeavySurroundRadius : FastSurroundRadius,
		MinAggressiveSurroundRadius);
	const FVector SlotDirection = PlayerForward.RotateAngleAxis(YawOffset, FVector::UpVector).GetSafeNormal2D();
	FVector ApproachTarget = PlayerTarget->GetActorLocation() + SlotDirection * TargetRadius;
	ProjectPointToNavigation(ApproachTarget);
	return ApproachTarget;
}

void AOBEnemy::DrawDetectionRadiusDebug() const
{
	if (!bShowEnemyDetectionRadii || bDead || DetectionRadius <= 0.0f || !GetWorld())
	{
		return;
	}

	const FVector Center = GetActorLocation() + FVector(0.0f, 0.0f, 8.0f);
	const FColor RadiusColor = EnemyType == EOBEnemyType::Heavy
		? FColor(255, 96, 64)
		: FColor(64, 180, 255);
	DrawDebugCircle(GetWorld(), Center, DetectionRadius, 96, RadiusColor, false, 0.0f, 0, 3.0f, FVector::ForwardVector, FVector::RightVector, false);
	DrawDebugPoint(GetWorld(), Center, 9.0f, RadiusColor, false, 0.0f, 0);
}

float AOBEnemy::GetEffectiveAttackRadius() const
{
	const UCapsuleComponent* EnemyCapsule = GetCapsuleComponent();
	const UCapsuleComponent* PlayerCapsule = PlayerTarget ? PlayerTarget->GetCapsuleComponent() : nullptr;
	const float EnemyRadius = EnemyCapsule ? EnemyCapsule->GetScaledCapsuleRadius() : 0.0f;
	const float PlayerRadius = PlayerCapsule ? PlayerCapsule->GetScaledCapsuleRadius() : 0.0f;
	const float ConfiguredRadius = EnemyType == EOBEnemyType::Heavy
		? HeavyAttackRadius
		: FastAttackRadius;
	return FMath::Max(ConfiguredRadius, EnemyRadius + PlayerRadius + TouchKillExtraMargin);
}

void AOBEnemy::DrawAttackRadiusDebug() const
{
	if (!bDrawAttackRadius || bDead || !GetWorld())
	{
		return;
	}

	const float AttackRadius = GetEffectiveAttackRadius();
	if (AttackRadius <= 0.0f)
	{
		return;
	}

	const FVector Center = GetActorLocation() + FVector::UpVector * 10.0f;
	const FColor RadiusColor = EnemyType == EOBEnemyType::Heavy
		? FColor(255, 48, 24)
		: FColor(255, 180, 32);
	DrawDebugCircle(
		GetWorld(),
		Center,
		AttackRadius,
		64,
		RadiusColor,
		false,
		0.0f,
		0,
		AttackRadiusDebugThickness,
		FVector::ForwardVector,
		FVector::RightVector,
		false);
	DrawDebugString(
		GetWorld(),
		Center + FVector::UpVector * 24.0f,
		FString::Printf(TEXT("Attack %.0f cm"), AttackRadius),
		nullptr,
		RadiusColor,
		0.0f,
		false,
		1.0f);
}

void AOBEnemy::DrawAIRoleTargetDebug() const
{
	if (!bDrawAIRoleTargets || bDead || !GetWorld() || CurrentApproachTarget.IsNearlyZero())
	{
		return;
	}

	FColor RoleColor = FColor::White;
	switch (CurrentRole)
	{
	case EOBEnemyRole::Chaser:
		RoleColor = FColor::Red;
		break;
	case EOBEnemyRole::Flanker:
		RoleColor = FColor::Yellow;
		break;
	case EOBEnemyRole::BulletBlocker:
		RoleColor = FColor::Cyan;
		break;
	default:
		break;
	}

	const FVector LineStart = GetActorLocation() + FVector::UpVector * 35.0f;
	const FVector Target = CurrentApproachTarget + FVector::UpVector * 20.0f;
	DrawDebugLine(GetWorld(), LineStart, Target, RoleColor, false, 0.0f, 0, 2.0f);
	DrawDebugSphere(GetWorld(), Target, AIRoleTargetDebugSize, 12, RoleColor, false, 0.0f, 0, 2.0f);
	DrawDebugString(
		GetWorld(),
		GetActorLocation() + FVector::UpVector * 135.0f,
		GetAIRoleName(CurrentRole),
		nullptr,
		RoleColor,
		0.0f,
		false,
		1.1f);
	DrawDebugString(
		GetWorld(),
		Target + FVector::UpVector * 24.0f,
		FString::Printf(TEXT("%s / %s"), GetAIStateName(CurrentAIState), GetAIRoleName(CurrentRole)),
		nullptr,
		RoleColor,
		0.0f,
		false,
		1.0f);
}

void AOBEnemy::TryTouchKill()
{
	if (bDead || bSpawnProtected || !PlayerTarget || PlayerTarget->IsDead())
	{
		return;
	}
	if (!IsPlayerDetected())
	{
		return;
	}

	const float EffectiveAttackRadius = GetEffectiveAttackRadius();

	if (!bCanTouchKillFromBehind)
	{
		FVector PlayerForward = PlayerTarget->GetActorForwardVector().GetSafeNormal2D();
		if (const AController* PlayerController = PlayerTarget->GetController())
		{
			PlayerForward = FRotationMatrix(FRotator(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X).GetSafeNormal2D();
		}

		const FVector ToEnemy = (GetActorLocation() - PlayerTarget->GetActorLocation()).GetSafeNormal2D();
		if (!ToEnemy.IsNearlyZero() && FVector::DotProduct(PlayerForward, ToEnemy) < TouchKillFrontMinDot)
		{
			return;
		}
	}

	if (FVector::DistSquared2D(GetActorLocation(), PlayerTarget->GetActorLocation()) <= FMath::Square(EffectiveAttackRadius))
	{
		PlayerTarget->DieWithReason(EnemyType == EOBEnemyType::Heavy
			? FText::FromString(TEXT("Crushed by Heavy"))
			: FText::FromString(TEXT("Caught by Fast")));
	}
}
