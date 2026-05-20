#include "OBCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "OBEnemy.h"
#include "OBGameMode.h"
#include "OBGameState.h"

AOBCharacter::AOBCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCharacterMovement()->MaxWalkSpeed = 680.0f;
	GetCharacterMovement()->JumpZVelocity = 520.0f;
	GetCharacterMovement()->AirControl = 0.45f;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(-10.0f, 0.0f, 70.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	FullBodyShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FullBodyShadowMesh"));
	FullBodyShadowMesh->SetupAttachment(GetCapsuleComponent());

	ConfigurePlayerMesh();
	ConfigureFullBodyShadowMesh();
}

void AOBCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>())
	{
		OneBulletState->SetBulletReady(true);
		OneBulletState->SetGameOver(false);
	}

	HideFirstPersonHead();
}

void AOBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDodge(DeltaSeconds);
}

void AOBCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AOBCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AOBCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AOBCharacter::LookYaw);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AOBCharacter::LookPitch);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Shoot"), IE_Pressed, this, &AOBCharacter::Shoot);
	PlayerInputComponent->BindAction(TEXT("Kick"), IE_Pressed, this, &AOBCharacter::Kick);
	PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &AOBCharacter::Dodge);
	PlayerInputComponent->BindAction(TEXT("Restart"), IE_Pressed, this, &AOBCharacter::RestartLevel);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AOBCharacter::Dodge);
	PlayerInputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &AOBCharacter::Dodge);
}

void AOBCharacter::MoveForward(float Value)
{
	if (!bDead && Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AOBCharacter::MoveRight(float Value)
{
	if (!bDead && Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AOBCharacter::LookYaw(float Value)
{
	if (!bDead)
	{
		AddControllerYawInput(Value);
	}
}

void AOBCharacter::LookPitch(float Value)
{
	if (!bDead)
	{
		AddControllerPitchInput(Value);
	}
}

void AOBCharacter::Shoot()
{
	if (bDead)
	{
		return;
	}

	AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>();
	if (!OneBulletState || !OneBulletState->HasBullet())
	{
		OnPlayerDryFire();
		if (DryFireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DryFireSound, GetActorLocation());
		}
		return;
	}

	OneBulletState->SetBulletReady(false);
	if (ShootSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ShootSound, GetActorLocation());
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * ShootRange;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OneBulletShoot), true, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	FVector BulletLocation = bHit ? Hit.ImpactPoint : End;
	const bool bHitEnemy = bHit && Cast<AOBEnemy>(Hit.GetActor()) != nullptr;
	OnPlayerShoot(Start, End, BulletLocation, bHit, bHitEnemy);
	if (bHit)
	{
		if (AOBEnemy* Enemy = Cast<AOBEnemy>(Hit.GetActor()))
		{
			Enemy->KillAndDropBullet(Enemy->GetActorLocation());
			return;
		}
	}

	DropBulletAt(BulletLocation);
}

void AOBCharacter::Kick()
{
	if (bDead || !bKickReady)
	{
		return;
	}

	bKickReady = false;
	GetWorldTimerManager().SetTimer(KickCooldownTimerHandle, this, &AOBCharacter::ResetKick, KickCooldown, false);
	if (KickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, KickSound, GetActorLocation());
	}

	const FVector Start = FirstPersonCamera->GetComponentLocation();
	const FVector End = Start + FirstPersonCamera->GetForwardVector() * KickRange;
	TArray<FHitResult> Hits;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(KickRadius);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OneBulletKick), false, this);
	GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, Sphere, Params);

	int32 HitEnemyCount = 0;
	for (const FHitResult& Hit : Hits)
	{
		if (AOBEnemy* Enemy = Cast<AOBEnemy>(Hit.GetActor()))
		{
			++HitEnemyCount;
			Enemy->ApplyKick(FirstPersonCamera->GetForwardVector());
		}
	}

	OnPlayerKick(Start, End, HitEnemyCount);
}

void AOBCharacter::Dodge()
{
	if (bDead || !bDodgeReady)
	{
		return;
	}

	FVector DodgeDirection = FVector::ZeroVector;
	if (!TryFindSafeDodgeDirection(DodgeDirection))
	{
		return;
	}

	bDodgeReady = false;
	GetWorldTimerManager().SetTimer(DodgeCooldownTimerHandle, this, &AOBCharacter::ResetDodge, DodgeCooldown, false);
	if (DodgeSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DodgeSound, GetActorLocation());
	}

	ActiveDodgeDirection = DodgeDirection;
	ActiveDodgeElapsed = 0.0f;
	ActiveDodgePreviousAlpha = 0.0f;
	bDodging = true;
	OnPlayerDodge(ActiveDodgeDirection);
}

void AOBCharacter::RecoverBullet()
{
	if (AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>())
	{
		OneBulletState->SetBulletReady(true);
	}
}

void AOBCharacter::Die()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	GetCharacterMovement()->DisableMovement();
	OnPlayerDeath();
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	if (AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>())
	{
		OneBulletState->SetGameOver(true);
	}
}

void AOBCharacter::RestartLevel()
{
	if (bDead)
	{
		const FName LevelName = *UGameplayStatics::GetCurrentLevelName(this, true);
		UGameplayStatics::OpenLevel(this, LevelName);
	}
}

void AOBCharacter::ResetKick()
{
	bKickReady = true;
}

void AOBCharacter::ResetDodge()
{
	bDodgeReady = true;
}

void AOBCharacter::UpdateDodge(float DeltaSeconds)
{
	if (!bDodging || bDead)
	{
		return;
	}

	ActiveDodgeElapsed += DeltaSeconds;
	const float Duration = FMath::Max(DodgeDuration, 0.01f);
	const float RawAlpha = FMath::Clamp(ActiveDodgeElapsed / Duration, 0.0f, 1.0f);
	const float SmoothedAlpha = FMath::Sin(RawAlpha * HALF_PI);
	const float AlphaStep = SmoothedAlpha - ActiveDodgePreviousAlpha;
	ActiveDodgePreviousAlpha = SmoothedAlpha;

	FHitResult Hit;
	AddActorWorldOffset(ActiveDodgeDirection * DodgeDistance * AlphaStep, true, &Hit);

	if (RawAlpha >= 1.0f || Hit.bBlockingHit)
	{
		bDodging = false;
	}
}

void AOBCharacter::PollDodgeInput()
{
	if (bDead || !DodgeKey.IsValid())
	{
		return;
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (PlayerController->WasInputKeyJustPressed(DodgeKey))
		{
			Dodge();
		}
	}
}

bool AOBCharacter::TryFindSafeDodgeDirection(FVector& OutDirection) const
{
	TArray<FVector> Candidates;
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	const FVector MovementDirection = GetLastMovementInputVector().GetSafeNormal2D();

	if (bPreferMovementDirectionDodge && !MovementDirection.IsNearlyZero())
	{
		Candidates.Add(MovementDirection);
	}

	Candidates.Add(Right);
	Candidates.Add(-Right);
	Candidates.Add(-Forward);
	Candidates.Add(Forward);

	float BestScore = -1.0f;
	for (const FVector& Candidate : Candidates)
	{
		if (Candidate.IsNearlyZero())
		{
			continue;
		}

		float CandidateScore = 0.0f;
		if (EvaluateDodgeDirection(Candidate.GetSafeNormal2D(), CandidateScore) && CandidateScore > BestScore)
		{
			BestScore = CandidateScore;
			OutDirection = Candidate.GetSafeNormal2D();
		}
	}

	return BestScore >= 0.0f;
}

bool AOBCharacter::EvaluateDodgeDirection(const FVector& Direction, float& OutScore) const
{
	const FVector Start = GetActorLocation();
	const FVector End = Start + Direction.GetSafeNormal2D() * DodgeDistance;
	const UCapsuleComponent* Capsule = GetCapsuleComponent();

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OneBulletDodgeProbe), false, this);
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		FMath::Max(Capsule->GetScaledCapsuleRadius() - 4.0f, 8.0f),
		FMath::Max(Capsule->GetScaledCapsuleHalfHeight() - 4.0f, 16.0f));

	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, CapsuleShape, Params) && Hit.bBlockingHit)
	{
		return false;
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AOBEnemy::StaticClass(), Enemies);

	float ClosestEnemyDistance = TNumericLimits<float>::Max();
	for (AActor* Actor : Enemies)
	{
		const AOBEnemy* Enemy = Cast<AOBEnemy>(Actor);
		if (!Enemy)
		{
			continue;
		}

		const FVector EnemyLocation = Enemy->GetActorLocation();
		const FVector ClosestPointOnPath = FMath::ClosestPointOnSegment(EnemyLocation, Start, End);
		const float PathDistance = FVector::Dist2D(EnemyLocation, ClosestPointOnPath);
		const float EndDistance = FVector::Dist2D(EnemyLocation, End);
		const float RequiredClearance = DodgeEnemyClearance + Enemy->TouchKillRadius;

		if (PathDistance < RequiredClearance || EndDistance < RequiredClearance)
		{
			return false;
		}

		ClosestEnemyDistance = FMath::Min(ClosestEnemyDistance, EndDistance);
	}

	const FVector PreferredDirection = GetLastMovementInputVector().GetSafeNormal2D();
	const float MovementBonus = (!PreferredDirection.IsNearlyZero()) ? FMath::Max(0.0f, FVector::DotProduct(Direction, PreferredDirection)) * 250.0f : 0.0f;
	OutScore = ClosestEnemyDistance + MovementBonus;
	return true;
}

void AOBCharacter::ConfigurePlayerMesh()
{
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCastShadow(false);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeScale3D(FVector(1.0f));
	GetMesh()->SetOwnerNoSee(false);

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

void AOBCharacter::ConfigureFullBodyShadowMesh()
{
	if (!FullBodyShadowMesh)
	{
		return;
	}

	FullBodyShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FullBodyShadowMesh->SetHiddenInGame(true);
	FullBodyShadowMesh->SetCastShadow(true);
	FullBodyShadowMesh->bCastHiddenShadow = true;
	FullBodyShadowMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	FullBodyShadowMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	FullBodyShadowMesh->SetRelativeScale3D(FVector(1.0f));

	if (GetMesh()->GetSkeletalMeshAsset())
	{
		FullBodyShadowMesh->SetSkeletalMesh(GetMesh()->GetSkeletalMeshAsset());
	}

	if (GetMesh()->GetAnimInstance())
	{
		FullBodyShadowMesh->SetAnimInstanceClass(GetMesh()->GetAnimInstance()->GetClass());
	}
	else if (GetMesh()->GetAnimClass())
	{
		FullBodyShadowMesh->SetAnimInstanceClass(GetMesh()->GetAnimClass());
	}
}

void AOBCharacter::HideFirstPersonHead()
{
	if (!bHideHeadForFirstPerson || !IsLocallyControlled() || !GetMesh())
	{
		return;
	}

	GetMesh()->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
	GetMesh()->HideBoneByName(TEXT("neck_01"), EPhysBodyOp::PBO_None);
}

void AOBCharacter::DropBulletAt(const FVector& Location)
{
	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		OneBulletMode->SpawnBulletPickup(Location);
	}
}
