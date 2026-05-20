#include "OBEnemy.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "OBCharacter.h"
#include "OBGameMode.h"
#include "OBGameState.h"

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
	if (!bDead && !bStunned && PlayerTarget && !PlayerTarget->IsDead())
	{
		const FVector Direction = (PlayerTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		AddMovementInput(Direction, 1.0f);
		SetActorRotation(Direction.Rotation());
	}
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
		OneBulletMode->SpawnBulletPickup(DropLocation);
	}
	Destroy();
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

void AOBEnemy::ResumeAfterStun()
{
	bStunned = false;
	RequestMove();
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
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->MoveToActor(PlayerTarget, 55.0f, true, true, true, nullptr, true);
		}
	}
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
		PlayerTarget->Die();
	}
}
