#include "OBEnemy.h"

#include "AIController.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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

namespace
{
bool IsAnimationCompatibleWithMesh(const UAnimationAsset* Animation, const USkeletalMeshComponent* MeshComponent)
{
	const USkeletalMesh* SkeletalMesh = MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr;
	return !Animation || !SkeletalMesh || !Animation->GetSkeleton() || Animation->GetSkeleton() == SkeletalMesh->GetSkeleton();
}
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
	NormalizePressureSettings();
	Configure(EnemyType);
	PlayerTarget = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	PatrolOrigin = PatrolCenter;
	PatrolOrigin.Z = GetActorLocation().Z;
	LastPatrolLocation = PatrolOrigin;
	bWasHoldingBullet = IsPlayerHoldingBullet();
	ApplyBulletPressureSpeed();
	GetWorldTimerManager().SetTimer(MoveTimerHandle, this, &AOBEnemy::RequestMove, 0.35f, true, 0.05f);
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

	ApplyBulletPressureSpeed();
	const bool bHoldingBullet = IsPlayerHoldingBullet();
	if (bWasHoldingBullet != bHoldingBullet)
	{
		bWasHoldingBullet = bHoldingBullet;
		bUsingDirectMovementFallback = false;
		bMovingToPatrolTarget = false;
		bHasPatrolTarget = false;
		CurrentPatrolPath.Reset();
		CurrentPatrolPathIndex = 0;
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}
		RequestMove();
	}

	if (PlayerTarget && PlayerTarget->IsDead())
	{
		StopPursuitForPlayerDeath();
		UpdateSimpleLocomotionAnimation();
		return;
	}

	bStoppedForPlayerDeath = false;
	if (!bStunned && PlayerTarget)
	{
		if (bHoldingBullet && ShouldPatrolWhilePlayerHasBullet())
		{
			bMovingToPatrolTarget = true;
			bUsingDirectMovementFallback = false;
			if (!bHasPatrolTarget)
			{
				ChooseNewPatrolTarget();
			}
			UpdatePatrolMovement(DeltaSeconds);
		}
		else if (bHoldingBullet)
		{
			bMovingToPatrolTarget = false;
			bHasPatrolTarget = false;
			CurrentApproachTarget = PlayerTarget->GetActorLocation();
			MoveToCurrentTarget(55.0f, false, true);
		}
		else if (!bHoldingBullet && bUseDirectLostBulletChase)
		{
			MoveAggressivelyToPlayer();
		}
		else if (bUsingDirectMovementFallback)
		{
			const FVector MovementDirection = (CurrentApproachTarget - GetActorLocation()).GetSafeNormal2D();
			if (!MovementDirection.IsNearlyZero())
			{
				AddMovementInput(MovementDirection, 1.0f, true);
			}
		}

		const FVector FacingTarget = bMovingToPatrolTarget ? CurrentApproachTarget : PlayerTarget->GetActorLocation();
		const FVector FacingDirection = (FacingTarget - GetActorLocation()).GetSafeNormal2D();
		if (!FacingDirection.IsNearlyZero())
		{
			SetActorRotation(FacingDirection.Rotation());
		}
	}
	UpdateSimpleLocomotionAnimation();
	TryTouchKill();
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

	ApplyBulletPressureSpeed();
}

void AOBEnemy::KillAndDropBullet(const FVector& DropLocation)
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	GetWorldTimerManager().ClearTimer(MoveTimerHandle);
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
		LaunchCharacter(LaunchDirection * 1400.0f + FVector(0.0f, 0.0f, 220.0f), true, true);
	}
	else
	{
		LaunchCharacter(LaunchDirection * 220.0f, true, false);
		bStunned = true;
		GetCharacterMovement()->StopMovementImmediately();
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
		}
		GetWorldTimerManager().SetTimer(StunTimerHandle, this, &AOBEnemy::ResumeAfterStun, 1.0f, false);
	}
}

void AOBEnemy::TriggerSpawnFeedback()
{
	OnEnemySpawned(EnemyType, GetActorLocation());
}

void AOBEnemy::Disappear()
{
	if (bDisappearing)
	{
		return;
	}

	bDisappearing = true;
	OnEnemyDisappearing(EnemyType, GetActorLocation());
	Destroy();
}

void AOBEnemy::ResumeAfterStun()
{
	bStunned = false;
	RequestMove();
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
	PatrolRadius = FMath::Max(PatrolRadius, 2600.0f);
	PatrolPointJitter = FMath::Max(PatrolPointJitter, 0.0f);
	PatrolMinTargetDistance = FMath::Clamp(PatrolMinTargetDistance, PatrolAcceptanceRadius, PatrolRadius * 0.75f);
	PatrolPerimeterRadiusMultiplier = FMath::Clamp(PatrolPerimeterRadiusMultiplier, 0.1f, 1.0f);
	PatrolPerimeterStepDegrees = FMath::Clamp(PatrolPerimeterStepDegrees, 5.0f, 180.0f);
	PatrolObstacleProbeDistance = FMath::Max(PatrolObstacleProbeDistance, 0.0f);
	PatrolObstacleProbeRadius = FMath::Max(PatrolObstacleProbeRadius, 0.0f);
}

void AOBEnemy::ApplyBulletPressureSpeed()
{
	const float BaseSpeed = EnemyType == EOBEnemyType::Heavy ? HeavySpeed : FastSpeed;
	float SpeedMultiplier = 1.0f;
	if (IsPlayerHoldingBullet())
	{
		SpeedMultiplier = IsPlayerInsideBulletAttackRadius()
			? PlayerHasBulletAttackSpeedMultiplier
			: PlayerHasBulletSpeedMultiplier;
	}
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * SpeedMultiplier;
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

bool AOBEnemy::ShouldPatrolWhilePlayerHasBullet() const
{
	if (!PlayerTarget || !IsPlayerHoldingBullet())
	{
		return false;
	}

	return !IsPlayerInsideBulletAttackRadius();
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
	if (SpeedSquared >= FMath::Square(RunAnimationMinSpeed))
	{
		DesiredAnimation = RunAnimation.Get();
		DesiredPlayRate = RunAnimationPlayRate;
	}
	else if (SpeedSquared >= FMath::Square(WalkAnimationMinSpeed))
	{
		DesiredAnimation = WalkAnimation ? WalkAnimation.Get() : RunAnimation.Get();
		DesiredPlayRate = WalkAnimationPlayRate;
	}

	if (DesiredAnimation && !IsAnimationCompatibleWithMesh(DesiredAnimation, GetMesh()))
	{
		if (DesiredAnimation == WalkAnimation.Get())
		{
			DesiredAnimation = RunAnimation.Get();
		}
		if (DesiredAnimation && !IsAnimationCompatibleWithMesh(DesiredAnimation, GetMesh()))
		{
			DesiredAnimation = IdleAnimation.Get();
		}
	}
	if (!DesiredAnimation || DesiredAnimation == ActiveLocomotionAnimation)
	{
		return;
	}

	ActiveLocomotionAnimation = DesiredAnimation;

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(DesiredAnimation, true);
	if (UAnimSingleNodeInstance* SingleNodeInstance = GetMesh()->GetSingleNodeInstance())
	{
		SingleNodeInstance->SetPlayRate(DesiredPlayRate);
	}
}

void AOBEnemy::RequestMove()
{
	if (bDead || bStunned)
	{
		return;
	}

	if (!PlayerTarget)
	{
		PlayerTarget = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	if (PlayerTarget && !PlayerTarget->IsDead())
	{
		if (ShouldPatrolWhilePlayerHasBullet())
		{
			bMovingToPatrolTarget = true;
			CurrentApproachTarget = GetOrChoosePatrolTarget();
		}
		else
		{
			if (IsPlayerHoldingBullet())
			{
				bMovingToPatrolTarget = false;
				bHasPatrolTarget = false;
				CurrentApproachTarget = PlayerTarget->GetActorLocation();
				MoveToCurrentTarget(55.0f, true, true);
			}
			else
			{
				MoveAggressivelyToPlayer();
			}
		}
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

void AOBEnemy::TryTouchKill()
{
	if (bDead || !PlayerTarget || PlayerTarget->IsDead())
	{
		return;
	}

	const UCapsuleComponent* EnemyCapsule = GetCapsuleComponent();
	const UCapsuleComponent* PlayerCapsule = PlayerTarget->GetCapsuleComponent();
	const float EnemyRadius = EnemyCapsule ? EnemyCapsule->GetScaledCapsuleRadius() : 0.0f;
	const float PlayerRadius = PlayerCapsule ? PlayerCapsule->GetScaledCapsuleRadius() : 0.0f;
	const float EffectiveTouchKillRadius = FMath::Max(TouchKillRadius, EnemyRadius + PlayerRadius + TouchKillExtraMargin);

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

	if (FVector::DistSquared2D(GetActorLocation(), PlayerTarget->GetActorLocation()) <= FMath::Square(EffectiveTouchKillRadius))
	{
		PlayerTarget->DieWithReason(EnemyType == EOBEnemyType::Heavy
			? FText::FromString(TEXT("Crushed by Heavy"))
			: FText::FromString(TEXT("Caught by Fast")));
	}
}
