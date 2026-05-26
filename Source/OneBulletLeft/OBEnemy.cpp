#include "OBEnemy.h"

#include "AIController.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NiagaraFunctionLibrary.h"
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
	Configure(EnemyType);
	PlayerTarget = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
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

	if (PlayerTarget && PlayerTarget->IsDead())
	{
		StopPursuitForPlayerDeath();
		UpdateSimpleLocomotionAnimation();
		return;
	}

	bStoppedForPlayerDeath = false;
	if (!bStunned && PlayerTarget)
	{
		if (bUsingDirectMovementFallback)
		{
			const FVector MovementDirection = (CurrentApproachTarget - GetActorLocation()).GetSafeNormal2D();
			if (!MovementDirection.IsNearlyZero())
			{
				AddMovementInput(MovementDirection);
			}
		}

		const FVector FacingDirection = (PlayerTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
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
		GetCharacterMovement()->MaxWalkSpeed = FastSpeed;
		GetCapsuleComponent()->SetCapsuleSize(38.0f, 92.0f);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -92.0f));
		GetMesh()->SetRelativeScale3D(FVector(0.92f, 0.92f, 0.92f));
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = HeavySpeed;
		GetCapsuleComponent()->SetCapsuleSize(58.0f, 110.0f);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -110.0f));
		GetMesh()->SetRelativeScale3D(FVector(1.25f, 1.25f, 1.25f));
	}
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
	if (SpawnEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, SpawnEffect, GetActorLocation(), GetActorRotation());
	}
	OnEnemySpawned(EnemyType);
}

void AOBEnemy::Disappear()
{
	if (bDisappearing)
	{
		return;
	}

	bDisappearing = true;
	if (DisappearEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, DisappearEffect, GetActorLocation(), GetActorRotation());
	}
	OnEnemyDisappearing(EnemyType);
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
	GetCharacterMovement()->StopMovementImmediately();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
}

void AOBEnemy::UpdateSimpleLocomotionAnimation()
{
	if (!bUseSimpleLocomotionAnimations || bDead || !GetMesh())
	{
		return;
	}

	UAnimationAsset* DesiredAnimation = GetVelocity().SizeSquared2D() >= FMath::Square(RunAnimationMinSpeed)
		? RunAnimation.Get()
		: IdleAnimation.Get();
	if (!DesiredAnimation || DesiredAnimation == ActiveLocomotionAnimation)
	{
		return;
	}

	ActiveLocomotionAnimation = DesiredAnimation;
	if (!IsAnimationCompatibleWithMesh(DesiredAnimation, GetMesh()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skipping locomotion animation %s: it uses a different skeleton than enemy %s."), *GetNameSafe(DesiredAnimation), *GetName());
		return;
	}

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(DesiredAnimation, true);
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
		CurrentApproachTarget = CalculateApproachTarget();
		bUsingDirectMovementFallback = false;
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			const EPathFollowingRequestResult::Type MoveResult = AI->MoveToLocation(CurrentApproachTarget, 20.0f, true, true, true, true, nullptr, true);
			bUsingDirectMovementFallback = bUseDirectMovementFallback && MoveResult == EPathFollowingRequestResult::Failed;
		}
		else
		{
			bUsingDirectMovementFallback = bUseDirectMovementFallback;
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
	const float TargetRadius = EnemyType == EOBEnemyType::Heavy ? HeavySurroundRadius : FastSurroundRadius;
	const FVector SlotDirection = PlayerForward.RotateAngleAxis(YawOffset, FVector::UpVector).GetSafeNormal2D();
	return PlayerTarget->GetActorLocation() + SlotDirection * TargetRadius;
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
