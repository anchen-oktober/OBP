#include "OBCharacter.h"

#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "OBEnemy.h"
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

	ThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonSpringArm"));
	ThirdPersonSpringArm->SetupAttachment(GetCapsuleComponent());
	ThirdPersonSpringArm->bUsePawnControlRotation = true;
	ThirdPersonSpringArm->TargetArmLength = ThirdPersonCameraDistance;
	ThirdPersonSpringArm->SocketOffset = ThirdPersonCameraOffset;
	ThirdPersonSpringArm->bEnableCameraLag = bThirdPersonCameraLag;
	ThirdPersonSpringArm->CameraLagSpeed = ThirdPersonCameraLagSpeed;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonSpringArm, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	ThirdPersonCamera->SetActive(false);

	FullBodyShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FullBodyShadowMesh"));
	FullBodyShadowMesh->SetupAttachment(GetCapsuleComponent());

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(FirstPersonCamera);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultWeapon(TEXT("/Game/Weapons/GrenadeLauncher/Meshes/SK_GrenadeLauncher.SK_GrenadeLauncher"));
	if (DefaultWeapon.Succeeded())
	{
		WeaponModel = DefaultWeapon.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultShootEffect(TEXT("/Game/MilitaryWeapDark/FX/P_Grenade_MuzzleFlash_01.P_Grenade_MuzzleFlash_01"));
	if (DefaultShootEffect.Succeeded())
	{
		ShootEffect = DefaultShootEffect.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultShootSound(TEXT("/Game/Weapons/GrenadeLauncher/Audio/FirstPersonTemplateWeaponFire02.FirstPersonTemplateWeaponFire02"));
	if (DefaultShootSound.Succeeded())
	{
		ShootSound = DefaultShootSound.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultBulletImpactSound(TEXT("/Game/Sound/cue/Rock_Impact_37_Cue.Rock_Impact_37_Cue"));
	if (DefaultBulletImpactSound.Succeeded())
	{
		BulletImpactSound = DefaultBulletImpactSound.Object;
	}

	static ConstructorHelpers::FObjectFinder<UParticleSystem> DefaultBulletImpactEffect(TEXT("/Game/Assets/VFX/Realistic_Starter_VFX_Pack_Vol2/Particles/Hit/P_Concrete.P_Concrete"));
	if (DefaultBulletImpactEffect.Succeeded())
	{
		BulletImpactEffect = DefaultBulletImpactEffect.Object;
	}

	ConfigurePlayerMesh();
	ConfigureFullBodyShadowMesh();
	ConfigureWeapon();
}

void AOBCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureWeapon();
	ApplyWeaponReadyTransform(true);
}

void AOBCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetMesh())
	{
		DefaultPlayerAnimClass = GetMesh()->GetAnimClass();
		ConfigureFullBodyShadowMesh();
	}
	ConfigureWeapon();

	if (AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>())
	{
		OneBulletState->SetBulletReady(true);
		OneBulletState->SetGameOver(false);
	}

	bThirdPersonView = bStartInThirdPerson;
	bImmortalMode = bStartImmortal;
	SetWeaponBulletReady(true, true);
	ApplyViewMode();
}

void AOBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDodge(DeltaSeconds);
	UpdateRecoil(DeltaSeconds);
	UpdateSimpleLocomotionAnimation();
	UpdateWeaponPose(DeltaSeconds);
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
	PlayerInputComponent->BindAction(TEXT("Restart"), IE_Pressed, this, &AOBCharacter::RestartLevel);
	if (DodgeKey.IsValid())
	{
		PlayerInputComponent->BindKey(DodgeKey, IE_Pressed, this, &AOBCharacter::Dodge);
	}
	if (SecondaryDodgeKey.IsValid() && SecondaryDodgeKey != DodgeKey)
	{
		PlayerInputComponent->BindKey(SecondaryDodgeKey, IE_Pressed, this, &AOBCharacter::Dodge);
	}
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &AOBCharacter::ToggleViewMode);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AOBCharacter::ToggleImmortalMode);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AOBCharacter::ToggleEnemyDetectionRadiusVisualization);
}

void AOBCharacter::MoveForward(float Value)
{
	LastMoveForwardInput = Value;

	if (!bDead && Controller && FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AOBCharacter::MoveRight(float Value)
{
	LastMoveRightInput = Value;

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

	FVector TraceStart = FVector::ZeroVector;
	FVector TraceEnd = FVector::ZeroVector;
	GetCrosshairTrace(TraceStart, TraceEnd);
	const FVector BulletVisualStart = GetBulletVisualStartLocation(TraceStart);

	OneBulletState->SetBulletReady(false);
	PlayActionAnimation(ShootAnimation, ShootAnimationDuration);
	PlayShootEffect();
	PlayWeaponShootAnimation();
	if (ShootSound)
	{
		const FVector SoundLocation = WeaponMesh
			? WeaponMesh->GetSocketLocation(ShootEffectSocketName)
			: GetActorLocation();
		UGameplayStatics::PlaySoundAtLocation(this, ShootSound, SoundLocation);
	}
	SetWeaponBulletReady(false);

	AddControllerPitchInput(-RecoilPitchImpulse);
	AddControllerYawInput(FMath::FRandRange(-RecoilYawRandomness, RecoilYawRandomness));
	RemainingRecoilPitch += RecoilPitchImpulse;

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (ShootCameraShake && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraShake(ShootCameraShake);
		}
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OneBulletShoot), true, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

	FVector BulletLocation = bHit ? Hit.ImpactPoint : TraceEnd;
	const bool bHitEnemy = bHit && Cast<AOBEnemy>(Hit.GetActor()) != nullptr;
	OnPlayerShoot(TraceStart, TraceEnd, BulletLocation, bHit, bHitEnemy);
	if (bHit)
	{
		if (AOBEnemy* Enemy = Cast<AOBEnemy>(Hit.GetActor()))
		{
			OnPlayerHitConfirmed(Hit.ImpactPoint);
			if (HitConfirmSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, HitConfirmSound, Hit.ImpactPoint);
			}
			ApplyFeelStop(HitStopDuration);
			Enemy->KillAndDropBullet(Hit.ImpactPoint);
			if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
			{
				OneBulletMode->PlayBulletFlight(BulletVisualStart, BulletLocation, Enemy->GetDroppedBulletPickup());
			}
			return;
		}
	}

	if (bHit)
	{
		PlayBulletImpactFeedback(Hit);
		BulletLocation = ResolveBulletDropLocationAfterImpact(Hit);
	}

	AOBBulletPickup* DroppedPickup = DropBulletAt(BulletLocation);
	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		OneBulletMode->PlayBulletFlight(BulletVisualStart, BulletLocation, DroppedPickup);
	}
}

void AOBCharacter::Kick()
{
	if (bDead || !bKickReady)
	{
		return;
	}

	bKickReady = false;
	GetWorldTimerManager().SetTimer(KickCooldownTimerHandle, this, &AOBCharacter::ResetKick, KickCooldown, false);
	PlayActionAnimation(KickAnimation, KickAnimationDuration);
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

	TArray<AOBEnemy*> HitEnemies;
	for (const FHitResult& Hit : Hits)
	{
		if (AOBEnemy* Enemy = Cast<AOBEnemy>(Hit.GetActor()))
		{
			HitEnemies.AddUnique(Enemy);
		}
	}

	const int32 HitEnemyCount = HitEnemies.Num();
	if (HitEnemyCount == 1)
	{
		const FVector AwayFromPlayer = (HitEnemies[0]->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		HitEnemies[0]->ApplyKick(AwayFromPlayer.IsNearlyZero() ? FirstPersonCamera->GetForwardVector() : AwayFromPlayer);
	}
	else if (HitEnemyCount > 1)
	{
		HitEnemies.Sort([this](const AOBEnemy& Left, const AOBEnemy& Right)
		{
			const FVector ToLeft = (Left.GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			const FVector ToRight = (Right.GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			return FVector::DotProduct(GetActorRightVector(), ToLeft) < FVector::DotProduct(GetActorRightVector(), ToRight);
		});

		const float SpreadDegrees = 70.0f;
		const float Step = HitEnemyCount > 1 ? SpreadDegrees / static_cast<float>(HitEnemyCount - 1) : 0.0f;
		for (int32 Index = 0; Index < HitEnemyCount; ++Index)
		{
			const float YawOffset = -SpreadDegrees * 0.5f + Step * static_cast<float>(Index);
			const FVector SpreadDirection = FirstPersonCamera->GetForwardVector().RotateAngleAxis(YawOffset, FVector::UpVector).GetSafeNormal2D();
			HitEnemies[Index]->ApplyKick(SpreadDirection);
		}
	}

	OnPlayerKick(Start, End, HitEnemyCount);
}

void AOBCharacter::Dodge()
{
	if (bDead)
	{
		return;
	}

	if (!bDodgeReady)
	{
		OnPlayerDodgeFailed(FText::FromString(TEXT("Dodge cooldown")));
		return;
	}

	FVector DodgeDirection = FVector::ZeroVector;
	if (!TryFindSafeDodgeDirection(DodgeDirection))
	{
		OnPlayerDodgeFailed(FText::FromString(TEXT("No safe space")));
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
		SetWeaponBulletReady(true);
	}
}

void AOBCharacter::ConfirmPickupFeedback(const FVector& PickupLocation)
{
	OnPlayerBulletRecovered(PickupLocation);
	ApplyFeelStop(PickupStopDuration);
}

void AOBCharacter::ToggleViewMode()
{
	SetThirdPersonView(!bThirdPersonView);
}

void AOBCharacter::SetThirdPersonView(bool bUseThirdPerson)
{
	if (bThirdPersonView == bUseThirdPerson)
	{
		return;
	}

	bThirdPersonView = bUseThirdPerson;
	ApplyViewMode();
	OnPlayerViewModeChanged(bThirdPersonView);
}

void AOBCharacter::SetCameraWeaponRelativeTransform(const FTransform& NewTransform)
{
	CameraWeaponRelativeTransform = NewTransform;
	if (bAttachWeaponToCamera)
	{
		ApplyWeaponReadyTransform(true);
	}
}

void AOBCharacter::ToggleImmortalMode()
{
	bImmortalMode = !bImmortalMode;
	OnPlayerImmortalModeChanged(bImmortalMode);
}

void AOBCharacter::ToggleEnemyDetectionRadiusVisualization()
{
	AOBEnemy::ToggleDetectionRadiusVisualization();
}

void AOBCharacter::ResetForNewRun(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	bDead = false;
	bKickReady = true;
	bDodgeReady = true;
	bDodging = false;
	ActiveDodgeDirection = FVector::ZeroVector;
	ActiveDodgeElapsed = 0.0f;
	ActiveDodgePreviousAlpha = 0.0f;
	RemainingRecoilPitch = 0.0f;
	SetWeaponBulletReady(true, true);

	GetWorldTimerManager().ClearTimer(KickCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(DodgeCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(ActionAnimationTimerHandle);
	RestoreMovementAnimation();
	ResetFeelStop();

	SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
	if (Controller)
	{
		Controller->SetControlRotation(SpawnRotation);
	}

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetCharacterMovement()->StopMovementImmediately();
}

void AOBCharacter::Die()
{
	DieWithReason(FText::FromString(TEXT("Caught by enemy")));
}

void AOBCharacter::DieWithReason(const FText& DeathReason)
{
	if (bDead || bImmortalMode)
	{
		return;
	}

	bDead = true;
	GetCharacterMovement()->DisableMovement();
	PlayDeathAnimation();
	OnPlayerDeath();
	if (DeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	}

	if (AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>())
	{
		OneBulletState->SetGameOverWithReason(true, DeathReason);
	}
}

void AOBCharacter::RestartLevel()
{
	if (bDead)
	{
		if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
		{
			OneBulletMode->RestartRun(this);
		}
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

void AOBCharacter::UpdateRecoil(float DeltaSeconds)
{
	if (RemainingRecoilPitch <= KINDA_SMALL_NUMBER || !Controller)
	{
		return;
	}

	const float Recovery = FMath::Min(RemainingRecoilPitch, RecoilRecoverySpeed * DeltaSeconds);
	AddControllerPitchInput(Recovery);
	RemainingRecoilPitch -= Recovery;
}

void AOBCharacter::ApplyFeelStop(float Duration)
{
	if (!GetWorld() || Duration <= 0.0f)
	{
		return;
	}

	UGameplayStatics::SetGlobalTimeDilation(this, HitStopTimeDilation);
	GetWorldTimerManager().ClearTimer(FeelStopTimerHandle);
	GetWorldTimerManager().SetTimer(FeelStopTimerHandle, this, &AOBCharacter::ResetFeelStop, Duration * HitStopTimeDilation, false);
}

void AOBCharacter::ResetFeelStop()
{
	GetWorldTimerManager().ClearTimer(FeelStopTimerHandle);
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
}

float AOBCharacter::GetDodgeCooldownRemaining() const
{
	return GetWorldTimerManager().GetTimerRemaining(DodgeCooldownTimerHandle);
}

float AOBCharacter::GetDodgeCooldownNormalized() const
{
	if (DodgeCooldown <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(GetDodgeCooldownRemaining() / DodgeCooldown, 0.0f, 1.0f);
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

bool AOBCharacter::TryFindSafeDodgeDirection(FVector& OutDirection) const
{
	const FVector MovementDirection = GetMovementInputDodgeDirection();
	if (MovementDirection.IsNearlyZero())
	{
		OutDirection = GetActorForwardVector().GetSafeNormal2D();
		return !OutDirection.IsNearlyZero();
	}

	OutDirection = MovementDirection;
	return true;
}

FVector AOBCharacter::GetMovementInputDodgeDirection() const
{
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = GetActorRightVector().GetSafeNormal2D();
	float ForwardInput = LastMoveForwardInput;
	float RightInput = LastMoveRightInput;

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		float DigitalForwardInput = 0.0f;
		float DigitalRightInput = 0.0f;

		if (PlayerController->IsInputKeyDown(EKeys::W) || PlayerController->IsInputKeyDown(EKeys::Up))
		{
			DigitalForwardInput += 1.0f;
		}
		if (PlayerController->IsInputKeyDown(EKeys::S) || PlayerController->IsInputKeyDown(EKeys::Down))
		{
			DigitalForwardInput -= 1.0f;
		}
		if (PlayerController->IsInputKeyDown(EKeys::D) || PlayerController->IsInputKeyDown(EKeys::Right))
		{
			DigitalRightInput += 1.0f;
		}
		if (PlayerController->IsInputKeyDown(EKeys::A) || PlayerController->IsInputKeyDown(EKeys::Left))
		{
			DigitalRightInput -= 1.0f;
		}

		if (FMath::Abs(DigitalForwardInput) > KINDA_SMALL_NUMBER)
		{
			ForwardInput = DigitalForwardInput;
		}
		if (FMath::Abs(DigitalRightInput) > KINDA_SMALL_NUMBER)
		{
			RightInput = DigitalRightInput;
		}
	}

	return (Forward * ForwardInput + Right * RightInput).GetSafeNormal2D();
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

	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, CapsuleShape, Params) && Hit.bBlockingHit)
	{
		return false;
	}

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AOBEnemy::StaticClass(), Enemies);

	float ClosestEnemyDistance = TNumericLimits<float>::Max();
	for (AActor* Actor : Enemies)
	{
		const AOBEnemy* Enemy = Cast<AOBEnemy>(Actor);
		if (!Enemy || Enemy->IsDead())
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

	OutScore = ClosestEnemyDistance;
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
		DefaultPlayerAnimClass = UnarmedAnimBP.Class;
		GetMesh()->SetAnimInstanceClass(UnarmedAnimBP.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DefaultKickAnimation(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
	if (DefaultKickAnimation.Succeeded())
	{
		KickAnimation = DefaultKickAnimation.Object;
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

void AOBCharacter::ConfigureWeapon()
{
	if (!WeaponMesh)
	{
		return;
	}

	if (USceneComponent* AttachParent = GetWeaponAttachParent())
	{
		const FName AttachSocket = bAttachWeaponToCamera ? NAME_None : WeaponAttachSocketName;
		WeaponMesh->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform, AttachSocket);
	}
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetCastShadow(true);
	if (WeaponModel)
	{
		WeaponMesh->SetSkeletalMesh(WeaponModel);
	}
}

void AOBCharacter::ApplyWeaponReadyTransform(bool bSnap)
{
	if (!WeaponMesh || !bWeaponBulletReady)
	{
		return;
	}

	if (bSnap || WeaponPoseBlendSpeed <= KINDA_SMALL_NUMBER)
	{
		WeaponMesh->SetRelativeTransform(GetWeaponReadyTargetTransform());
	}
}

void AOBCharacter::SetWeaponBulletReady(bool bReady, bool bSnap)
{
	const bool bStateChanged = bWeaponBulletReady != bReady;
	bWeaponBulletReady = bReady;
	if (WeaponMesh)
	{
		WeaponMesh->SetHiddenInGame(!bReady, true);
		WeaponMesh->SetVisibility(bReady, true);
		if (bReady && bSnap)
		{
			ApplyWeaponReadyTransform(true);
		}
	}

	if (bStateChanged && bUseSimpleLocomotionAnimations && !bDead && !bPlayingActionAnimation)
	{
		ActiveLocomotionAnimation = nullptr;
		UpdateSimpleLocomotionAnimation();
	}

	OnPlayerWeaponStateChanged(bReady);
}

void AOBCharacter::UpdateWeaponPose(float DeltaSeconds)
{
	if (!WeaponMesh || !bWeaponBulletReady)
	{
		return;
	}

	const FTransform& TargetTransform = GetWeaponReadyTargetTransform();
	if (WeaponPoseBlendSpeed <= KINDA_SMALL_NUMBER)
	{
		WeaponMesh->SetRelativeTransform(TargetTransform);
		return;
	}

	const float Alpha = FMath::Clamp(WeaponPoseBlendSpeed * DeltaSeconds, 0.0f, 1.0f);
	FTransform SmoothedTransform;
	SmoothedTransform.Blend(WeaponMesh->GetRelativeTransform(), TargetTransform, Alpha);
	WeaponMesh->SetRelativeTransform(SmoothedTransform);
}

USceneComponent* AOBCharacter::GetWeaponAttachParent() const
{
	if (bAttachWeaponToCamera && FirstPersonCamera)
	{
		return FirstPersonCamera;
	}

	return GetMesh();
}

const FTransform& AOBCharacter::GetWeaponReadyTargetTransform() const
{
	return bAttachWeaponToCamera ? CameraWeaponRelativeTransform : WeaponReadyRelativeTransform;
}

void AOBCharacter::PlayShootEffect()
{
	if (!ShootEffect || !WeaponMesh)
	{
		return;
	}

	const FTransform MuzzleTransform = WeaponMesh->DoesSocketExist(ShootEffectSocketName)
		? WeaponMesh->GetSocketTransform(ShootEffectSocketName)
		: WeaponMesh->GetComponentTransform();
	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		ShootEffect,
		MuzzleTransform,
		true,
		EPSCPoolMethod::AutoRelease);
}

void AOBCharacter::PlayBulletImpactFeedback(const FHitResult& Hit) const
{
	const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
	const FVector ImpactLocation = Hit.ImpactPoint + ImpactNormal * 4.0f;
	if (BulletImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BulletImpactSound, ImpactLocation);
	}
	if (BulletImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			BulletImpactEffect,
			ImpactLocation,
			ImpactNormal.Rotation(),
			true,
			EPSCPoolMethod::AutoRelease);
	}
}

void AOBCharacter::PlayWeaponShootAnimation()
{
	if (!WeaponShootAnimation || !WeaponMesh || !IsAnimationCompatibleWithMesh(WeaponShootAnimation, WeaponMesh))
	{
		if (WeaponShootAnimation && WeaponMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("Skipping weapon shoot animation %s: it uses a different skeleton than weapon %s."), *GetNameSafe(WeaponShootAnimation), *GetNameSafe(WeaponModel));
		}
		return;
	}

	WeaponMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	WeaponMesh->PlayAnimation(WeaponShootAnimation, false);
}

FVector AOBCharacter::GetBulletVisualStartLocation(const FVector& TraceStart) const
{
	if (!bUseWeaponMuzzleForBulletFlight || !WeaponMesh)
	{
		return TraceStart;
	}

	return WeaponMesh->DoesSocketExist(ShootEffectSocketName)
		? WeaponMesh->GetSocketLocation(ShootEffectSocketName)
		: WeaponMesh->GetComponentLocation();
}

FVector AOBCharacter::ResolveBulletDropLocationAfterImpact(const FHitResult& Hit) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return Hit.ImpactPoint;
	}

	const FVector IncomingDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
	const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
	FVector RicochetDirection = FMath::GetReflectionVector(IncomingDirection, ImpactNormal).GetSafeNormal2D();
	if (RicochetDirection.IsNearlyZero())
	{
		RicochetDirection = ImpactNormal.GetSafeNormal2D();
	}
	if (RicochetDirection.IsNearlyZero())
	{
		RicochetDirection = -IncomingDirection.GetSafeNormal2D();
	}
	if (RicochetDirection.IsNearlyZero())
	{
		RicochetDirection = GetActorForwardVector().GetSafeNormal2D();
	}

	const float WallClearance = FMath::Max(BulletRicochetWallClearance, 145.0f);
	const float DropDistance = FMath::Max(BulletRicochetDropDistance, 180.0f);
	const FVector OutFromWall = ImpactNormal * WallClearance;
	const FVector FloorTraceLift = FVector::UpVector * FMath::Max(BulletRicochetUpLift, 60.0f);
	const float FloorTraceDistance = FMath::Max(BulletRicochetFloorTraceDistance, 600.0f);

	TArray<FVector> CandidateDirections;
	CandidateDirections.Add(RicochetDirection);
	CandidateDirections.Add((RicochetDirection + ImpactNormal.GetSafeNormal2D()).GetSafeNormal2D());
	CandidateDirections.Add(ImpactNormal.GetSafeNormal2D());

	FCollisionQueryParams Params(SCENE_QUERY_STAT(OneBulletRicochetDrop), false, this);
	for (const FVector& CandidateDirection : CandidateDirections)
	{
		if (CandidateDirection.IsNearlyZero())
		{
			continue;
		}

		for (const float DistanceScale : {1.0f, 1.45f})
		{
			const FVector ProbeCenter = Hit.ImpactPoint + OutFromWall + CandidateDirection * DropDistance * DistanceScale + FloorTraceLift;
			const FVector FloorTraceStart = ProbeCenter + FVector::UpVector * 120.0f;
			const FVector FloorTraceEnd = ProbeCenter - FVector::UpVector * FloorTraceDistance;

			FHitResult FloorHit;
			if (World->LineTraceSingleByChannel(FloorHit, FloorTraceStart, FloorTraceEnd, ECC_Visibility, Params)
				&& FloorHit.bBlockingHit
				&& FloorHit.ImpactNormal.Z >= 0.55f)
			{
				return FloorHit.ImpactPoint;
			}
		}
	}

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float PlayerFloorZ = GetActorLocation().Z - (Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 96.0f);
	const FVector Fallback = Hit.ImpactPoint + OutFromWall + RicochetDirection * DropDistance;
	return FVector(Fallback.X, Fallback.Y, PlayerFloorZ);
}

void AOBCharacter::HideFirstPersonHead()
{
	if (!bHideHeadForFirstPerson || !IsLocallyControlled() || !GetMesh())
	{
		return;
	}

	GetMesh()->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
}

void AOBCharacter::PlayActionAnimation(UAnimationAsset* Animation, float Duration)
{
	if (!Animation || !GetMesh())
	{
		return;
	}

	if (!IsAnimationCompatibleWithMesh(Animation, GetMesh()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skipping action animation %s: it uses a different skeleton than the player mesh."), *GetNameSafe(Animation));
		return;
	}

	bPlayingActionAnimation = true;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(Animation, false);
	if (FullBodyShadowMesh)
	{
		FullBodyShadowMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		FullBodyShadowMesh->PlayAnimation(Animation, false);
	}
	GetWorldTimerManager().SetTimer(ActionAnimationTimerHandle, this, &AOBCharacter::RestoreMovementAnimation, FMath::Max(Duration, 0.01f), false);
}

void AOBCharacter::PlayDeathAnimation()
{
	if (!DeathAnimation || !GetMesh() || !IsAnimationCompatibleWithMesh(DeathAnimation, GetMesh()))
	{
		if (DeathAnimation && GetMesh())
		{
			UE_LOG(LogTemp, Warning, TEXT("Skipping death animation %s: it uses a different skeleton than the player mesh."), *GetNameSafe(DeathAnimation));
		}
		return;
	}

	GetWorldTimerManager().ClearTimer(ActionAnimationTimerHandle);
	bPlayingActionAnimation = true;
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(DeathAnimation, false);
	if (FullBodyShadowMesh)
	{
		FullBodyShadowMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		FullBodyShadowMesh->PlayAnimation(DeathAnimation, false);
	}
}

void AOBCharacter::UpdateSimpleLocomotionAnimation()
{
	if (!bUseSimpleLocomotionAnimations || bDead || bPlayingActionAnimation || !GetMesh())
	{
		return;
	}

	const bool bHasMovementInput = !GetPendingMovementInputVector().IsNearlyZero() || !GetLastMovementInputVector().IsNearlyZero();
	const bool bRunning = bHasMovementInput && GetVelocity().SizeSquared2D() >= FMath::Square(RunAnimationMinSpeed);
	UAnimationAsset* DesiredRunAnimation = bWeaponBulletReady && PistolRunAnimation
		? PistolRunAnimation.Get()
		: RunAnimation.Get();
	UAnimationAsset* DesiredIdleAnimation = bWeaponBulletReady && PistolIdleAnimation
		? PistolIdleAnimation.Get()
		: IdleAnimation.Get();
	UAnimationAsset* DesiredAnimation = bRunning ? DesiredRunAnimation : DesiredIdleAnimation;
	if (!DesiredAnimation || DesiredAnimation == ActiveLocomotionAnimation)
	{
		return;
	}

	ActiveLocomotionAnimation = DesiredAnimation;
	if (!IsAnimationCompatibleWithMesh(DesiredAnimation, GetMesh()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Skipping locomotion animation %s: it uses a different skeleton than the player mesh."), *GetNameSafe(DesiredAnimation));
		return;
	}

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->PlayAnimation(DesiredAnimation, true);
	if (FullBodyShadowMesh)
	{
		FullBodyShadowMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		FullBodyShadowMesh->PlayAnimation(DesiredAnimation, true);
	}
}

void AOBCharacter::RestoreMovementAnimation()
{
	bPlayingActionAnimation = false;
	ActiveLocomotionAnimation = nullptr;
	if (bUseSimpleLocomotionAnimations)
	{
		UpdateSimpleLocomotionAnimation();
		return;
	}

	if (!DefaultPlayerAnimClass)
	{
		return;
	}

	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	GetMesh()->SetAnimInstanceClass(DefaultPlayerAnimClass);
	if (FullBodyShadowMesh)
	{
		FullBodyShadowMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		FullBodyShadowMesh->SetAnimInstanceClass(DefaultPlayerAnimClass);
	}
}

void AOBCharacter::ApplyViewMode()
{
	if (!FirstPersonCamera || !ThirdPersonCamera || !ThirdPersonSpringArm)
	{
		return;
	}

	ThirdPersonSpringArm->TargetArmLength = ThirdPersonCameraDistance;
	ThirdPersonSpringArm->SocketOffset = ThirdPersonCameraOffset;
	ThirdPersonSpringArm->bEnableCameraLag = bThirdPersonCameraLag;
	ThirdPersonSpringArm->CameraLagSpeed = ThirdPersonCameraLagSpeed;
	FirstPersonCamera->SetActive(!bThirdPersonView);
	ThirdPersonCamera->SetActive(bThirdPersonView);

	if (!GetMesh())
	{
		return;
	}

	GetMesh()->SetOwnerNoSee(!bThirdPersonView && bAttachWeaponToCamera && bHideBodyForFirstPersonCameraWeapon);

	if (!IsLocallyControlled())
	{
		return;
	}

	if (bThirdPersonView)
	{
		GetMesh()->UnHideBoneByName(TEXT("head"));
	}
	else if (!bAttachWeaponToCamera || !bHideBodyForFirstPersonCameraWeapon)
	{
		HideFirstPersonHead();
	}
}

UCameraComponent* AOBCharacter::GetShootingCamera() const
{
	return bThirdPersonView && ThirdPersonCamera ? ThirdPersonCamera : FirstPersonCamera;
}

void AOBCharacter::GetCrosshairTrace(FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	const UCameraComponent* ShootingCamera = GetShootingCamera();
	OutTraceStart = ShootingCamera ? ShootingCamera->GetComponentLocation() : GetActorLocation();
	FVector AimDirection = ShootingCamera ? ShootingCamera->GetForwardVector() : GetActorForwardVector();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		int32 ViewportX = 0;
		int32 ViewportY = 0;
		PlayerController->GetViewportSize(ViewportX, ViewportY);
		if (ViewportX > 0 && ViewportY > 0)
		{
			FVector WorldLocation = FVector::ZeroVector;
			FVector WorldDirection = FVector::ZeroVector;
			if (PlayerController->DeprojectScreenPositionToWorld(
				static_cast<float>(ViewportX) * 0.5f,
				static_cast<float>(ViewportY) * 0.5f,
				WorldLocation,
				WorldDirection))
			{
				OutTraceStart = WorldLocation;
				AimDirection = WorldDirection.GetSafeNormal();
			}
		}
	}

	OutTraceEnd = OutTraceStart + AimDirection * ShootRange;
}

AOBBulletPickup* AOBCharacter::DropBulletAt(const FVector& Location)
{
	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		return OneBulletMode->SpawnBulletPickup(Location);
	}
	return nullptr;
}
