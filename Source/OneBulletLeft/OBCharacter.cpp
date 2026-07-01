#include "OBCharacter.h"

#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "AIController.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UObjectIterator.h"
#include "Particles/ParticleSystem.h"
#include "OBBulletPickup.h"
#include "OBBulletTrailActor.h"
#include "OBEnemy.h"
#include "OBGameMode.h"
#include "OBGameState.h"
#include "OBHUD.h"
#include "OBPanicAudioManager.h"
#include "OBWaveManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogOBPerformance, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogOBKickVisual, Log, All);

namespace
{
constexpr float DefaultMouseSensitivity = 1.0f;
constexpr float MouseSensitivityDisplayPrecision = 100.0f;
const TCHAR* MouseSettingsSection = TEXT("OneBulletLeft.Controls");
const TCHAR* MouseSensitivityKey = TEXT("MouseSensitivity");

bool IsAnimationCompatibleWithMesh(const UAnimationAsset* Animation, const USkeletalMeshComponent* MeshComponent)
{
	const USkeletalMesh* SkeletalMesh = MeshComponent ? MeshComponent->GetSkeletalMeshAsset() : nullptr;
	return !Animation || !SkeletalMesh || !Animation->GetSkeleton() || Animation->GetSkeleton() == SkeletalMesh->GetSkeleton();
}

bool IsDisallowedKickLegMesh(const USkeletalMesh* SkeletalMesh)
{
	if (!SkeletalMesh)
	{
		return false;
	}

	const FString MeshPath = SkeletalMesh->GetPathName();
	return MeshPath == TEXT("/Game/Assets/Player.Player")
		|| MeshPath.Contains(TEXT("/Characters/Mannequins/Meshes/SKM_Manny"));
}

bool IsVisibleUserWidget(const UUserWidget* Widget)
{
	return Widget
		&& Widget->IsInViewport()
		&& Widget->GetVisibility() != ESlateVisibility::Collapsed
		&& Widget->GetVisibility() != ESlateVisibility::Hidden;
}

const TCHAR* GetKickResultName(EOBKickResult Result)
{
	switch (Result)
	{
	case EOBKickResult::BlockedByCooldown:
		return TEXT("BlockedByCooldown");
	case EOBKickResult::BlockedByDeath:
		return TEXT("BlockedByDeath");
	case EOBKickResult::Miss:
		return TEXT("Miss");
	case EOBKickResult::Hit:
		return TEXT("Hit");
	default:
		return TEXT("Unknown");
	}
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

	FullBodyShadowMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FullBodyShadowMesh"));
	FullBodyShadowMesh->SetupAttachment(GetCapsuleComponent());

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(FirstPersonCamera);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Player_Leg = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Player_Leg"));
	Player_Leg->SetupAttachment(FirstPersonCamera);
	Player_Leg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Player_Leg->SetOnlyOwnerSee(true);
	Player_Leg->SetOwnerNoSee(false);
	Player_Leg->SetCastShadow(false);
	Player_Leg->bCastHiddenShadow = false;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultWeapon(TEXT("/Game/Weapons/GrenadeLauncher/Meshes/SK_GrenadeLauncher.SK_GrenadeLauncher"));
	if (DefaultWeapon.Succeeded())
	{
		WeaponModel = DefaultWeapon.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> DefaultKickLegAnimation(TEXT("/Game/Assets/Animations/KickingLeg_Anim.KickingLeg_Anim"));
	if (DefaultKickLegAnimation.Succeeded())
	{
		KickAnimation = DefaultKickLegAnimation.Object;
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
	ConfigureFirstPersonBodyVisibility();
	ConfigureKickLeg();
	ConfigureWeapon();
}

void AOBCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureKickLeg();
	ConfigureWeapon();
	ApplyWeaponReadyTransform(true);
}

void AOBCharacter::BeginPlay()
{
	Super::BeginPlay();
	LoadMouseSensitivity();

	if (FirstPersonCamera)
	{
		DefaultFirstPersonFOV = FirstPersonCamera->FieldOfView;
	}
	if (GetMesh())
	{
		DefaultPlayerAnimClass = GetMesh()->GetAnimClass();
		ConfigureFullBodyShadowMesh();
		ConfigureFirstPersonBodyVisibility();
	}
	ConfigureWeapon();
	ConfigureKickLeg();
	if (Player_Leg)
	{
		KickLegInitialRelativeTransform = Player_Leg->GetRelativeTransform();
		UE_LOG(
			LogOBKickVisual,
			Log,
			TEXT("Kick visual BeginPlay: CharacterClass=%s ExpectedRuntimeCharacterClass=%s Player_Leg=%s Parent=%s bOverrideKickLegTransformFromVariables=%s bKickVisualCalibrationMode=%s InitialRelativeTransform=%s WorldTransform=%s"),
			*GetClass()->GetName(),
			*GetNameSafe(ExpectedRuntimeCharacterClass.Get()),
			*GetNameSafe(Player_Leg),
			*GetNameSafe(Player_Leg->GetAttachParent()),
			bOverrideKickLegTransformFromVariables ? TEXT("true") : TEXT("false"),
			bKickVisualCalibrationMode ? TEXT("true") : TEXT("false"),
			*KickLegInitialRelativeTransform.ToHumanReadableString(),
			*Player_Leg->GetComponentTransform().ToHumanReadableString());

		if (ExpectedRuntimeCharacterClass && !IsA(ExpectedRuntimeCharacterClass))
		{
			UE_LOG(LogOBKickVisual, Warning, TEXT("Kick visual BeginPlay: Runtime character class %s is not using expected BP class %s."), *GetClass()->GetName(), *GetNameSafe(ExpectedRuntimeCharacterClass.Get()));
		}
	}
	else
	{
		UE_LOG(LogOBKickVisual, Warning, TEXT("Kick visual BeginPlay: Player_Leg component was not found on %s."), *GetName());
	}

	if (AOBGameState* OneBulletState = GetWorld()->GetGameState<AOBGameState>())
	{
		OneBulletState->SetBulletReady(true);
		OneBulletState->SetGameOver(false);
	}

	bImmortalMode = bStartImmortal;
	SetWeaponBulletReady(true, true);
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(true);
	}
	if (GetMesh())
	{
		ConfigureFirstPersonBodyVisibility();
		HideFirstPersonHead();
	}

	const bool bStartPerformanceDebug = bPerformanceDebugEnabled;
	bPerformanceDebugEnabled = false;
	if (bStartPerformanceDebug)
	{
		SetPerformanceDebugEnabled(true);
	}
}

void AOBCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateDodge(DeltaSeconds);
	UpdateKickDash(DeltaSeconds);
	UpdateRecoil(DeltaSeconds);
	UpdateKickCameraTilt(DeltaSeconds);
	UpdateKickFOVPunch(DeltaSeconds);
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
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AOBCharacter::ToggleImmortalMode);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AOBCharacter::ToggleMouseSensitivityUI);
	PlayerInputComponent->BindKey(EKeys::LeftBracket, IE_Pressed, this, &AOBCharacter::DecreaseMouseSensitivity);
	PlayerInputComponent->BindKey(EKeys::RightBracket, IE_Pressed, this, &AOBCharacter::IncreaseMouseSensitivity);
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
		AddControllerYawInput(Value * MouseSensitivity);
	}
}

void AOBCharacter::LookPitch(float Value)
{
	if (!bDead)
	{
		AddControllerPitchInput(Value * MouseSensitivity);
	}
}

void AOBCharacter::SetMouseSensitivity(float NewSensitivity)
{
	const float ClampedSensitivity = GetClampedMouseSensitivity(NewSensitivity);
	if (FMath::IsNearlyEqual(MouseSensitivity, ClampedSensitivity, 0.001f))
	{
		return;
	}

	MouseSensitivity = ClampedSensitivity;
	SaveMouseSensitivity();
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (AOBHUD* OneBulletHUD = Cast<AOBHUD>(PlayerController->GetHUD()))
		{
			OneBulletHUD->ShowMouseSensitivityChanged(MouseSensitivity);
		}
	}
}

void AOBCharacter::SetMouseSensitivityNormalized(float NormalizedValue)
{
	const float MinSensitivity = FMath::Max(MinMouseSensitivity, 0.01f);
	const float MaxSensitivity = FMath::Max(MaxMouseSensitivity, MinSensitivity);
	SetMouseSensitivity(FMath::Lerp(MinSensitivity, MaxSensitivity, FMath::Clamp(NormalizedValue, 0.0f, 1.0f)));
}

void AOBCharacter::AdjustMouseSensitivity(float Delta)
{
	SetMouseSensitivity(MouseSensitivity + Delta);
}

void AOBCharacter::ResetMouseSensitivity()
{
	SetMouseSensitivity(DefaultMouseSensitivity);
}

float AOBCharacter::GetMouseSensitivityNormalized() const
{
	const float MinSensitivity = FMath::Max(MinMouseSensitivity, 0.01f);
	const float MaxSensitivity = FMath::Max(MaxMouseSensitivity, MinSensitivity);
	if (FMath::IsNearlyEqual(MinSensitivity, MaxSensitivity))
	{
		return 1.0f;
	}

	return FMath::Clamp((MouseSensitivity - MinSensitivity) / (MaxSensitivity - MinSensitivity), 0.0f, 1.0f);
}

void AOBCharacter::IncreaseMouseSensitivity()
{
	AdjustMouseSensitivity(FMath::Max(MouseSensitivityStep, 0.01f));
}

void AOBCharacter::DecreaseMouseSensitivity()
{
	AdjustMouseSensitivity(-FMath::Max(MouseSensitivityStep, 0.01f));
}

void AOBCharacter::ToggleMouseSensitivityUI()
{
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (AOBHUD* OneBulletHUD = Cast<AOBHUD>(PlayerController->GetHUD()))
		{
			OneBulletHUD->ToggleMouseSensitivityPanel();
		}
	}
}

void AOBCharacter::Shoot()
{
	if (bDead || bKickRecovering)
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
	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		OneBulletMode->HandlePlayerShot(this);
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
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Kick input pressed. CurrentTime=%.3f NextAllowedKickTime=%.3f CooldownRemaining=%.3f PlayerDead=%s KickReady=%s KickRecovering=%s"),
		GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f,
		NextAllowedKickTime,
		GetWorld() ? FMath::Max(0.0f, NextAllowedKickTime - GetWorld()->GetTimeSeconds()) : -1.0f,
		bDead ? TEXT("true") : TEXT("false"),
		bKickReady ? TEXT("true") : TEXT("false"),
		bKickRecovering ? TEXT("true") : TEXT("false"));

	const EOBKickResult KickResult = GameplayKick();
	const bool bGameplayExecuted = KickResult == EOBKickResult::Hit || KickResult == EOBKickResult::Miss;
	if (!bGameplayExecuted)
	{
		UE_LOG(LogTemp, Warning, TEXT("Kick result: %s. GameplayKick executed=false Visual started=false"), GetKickResultName(KickResult));
		return;
	}

	bKickReady = false;
	bKickRecovering = true;
	GetWorldTimerManager().SetTimer(KickCooldownTimerHandle, this, &AOBCharacter::ResetKick, FMath::Max(KickCooldown, 0.0f), false);
	GetWorldTimerManager().SetTimer(KickRecoveryTimerHandle, this, &AOBCharacter::FinishKickRecovery, FMath::Max(KickRecoveryTime, 0.01f), false);
	const bool bVisualStarted = PlayKickLegAnimation();
	StartKickWeaponSway();
	PlayKickStartFeedback();
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Kick result: %s. GameplayKick executed=true Visual started=%s"),
		GetKickResultName(KickResult),
		bVisualStarted ? TEXT("true") : TEXT("false"));
}

EOBKickResult AOBCharacter::GameplayKick()
{
	if (bDead)
	{
		return EOBKickResult::BlockedByDeath;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Now < NextAllowedKickTime)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Kick blocked by cooldown. CurrentTime=%.3f NextAllowedKickTime=%.3f CooldownRemaining=%.3f"),
			Now,
			NextAllowedKickTime,
			FMath::Max(0.0f, NextAllowedKickTime - Now));
		return EOBKickResult::BlockedByCooldown;
	}
	NextAllowedKickTime = Now + FMath::Max(KickCooldown, 0.0f);

	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector PushForward = FVector::ForwardVector;
	FVector OverlapCenter = FVector::ZeroVector;
	float OverlapRadius = 0.0f;
	int32 CandidateCount = 0;
	FString TargetDebugText;
	TArray<AOBEnemy*> HitEnemies = GatherKickTargets(Start, End, PushForward, OverlapCenter, OverlapRadius, CandidateCount, TargetDebugText);

	AOBEnemy* SelectedEnemy = nullptr;
	float SelectedDistance = TNumericLimits<float>::Max();
	float SelectedDot = -1.0f;
	const float LogMinForwardDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(KickHalfAngleDegrees, 1.0f, 90.0f)));
	for (AOBEnemy* Enemy : HitEnemies)
	{
		if (!Enemy)
		{
			continue;
		}

		float Distance = 0.0f;
		float Dot = -1.0f;
		IsEnemyInKickZone(Enemy, Start, PushForward, LogMinForwardDot, Distance, Dot);
		if (Distance < SelectedDistance)
		{
			SelectedEnemy = Enemy;
			SelectedDistance = Distance;
			SelectedDot = Dot;
		}

		const FVector AwayFromPlayer = (Enemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		Enemy->ApplyKick(
			AwayFromPlayer.IsNearlyZero() ? PushForward : AwayFromPlayer,
			KickKnockbackStrength,
			KickPushDuration,
			KickStunDuration,
			KickSlowMultiplier,
			KickSlowDuration);
	}

	const int32 HitEnemyCount = HitEnemies.Num();
	const bool bHit = HitEnemyCount > 0;
	if (bHit)
	{
		LastSuccessfulKickTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastSuccessfulKickTime;
		LastSuccessfulKickEnemy = SelectedEnemy;
	}
	PlayKickImpactFeedback(GetActorLocation(), PushForward, HitEnemyCount);
	if (HitEnemyCount > 0 && KickImpactHitStopDuration > KINDA_SMALL_NUMBER)
	{
		ApplyFeelStop(KickImpactHitStopDuration);
	}
	OnPlayerKick(Start, End, HitEnemyCount);

	if (bKickDebugDraw)
	{
		const FColor DebugColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), Start, End, DebugColor, false, 1.25f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), Start, KickRadius, 16, DebugColor, false, 1.25f);
		DrawDebugSphere(GetWorld(), End, KickRadius, 16, DebugColor, false, 1.25f);
		DrawDebugSphere(GetWorld(), OverlapCenter, OverlapRadius, 24, FColor::Cyan, false, 1.25f);
		for (AOBEnemy* Enemy : HitEnemies)
		{
			if (Enemy)
			{
				DrawDebugLine(GetWorld(), Start, Enemy->GetActorLocation(), FColor::Green, false, 1.25f, 0, 1.5f);
				DrawDebugSphere(GetWorld(), Enemy->GetActorLocation(), 24.0f, 8, FColor::Green, false, 1.25f);
			}
		}

		const float ConeLength = FMath::Max(KickRange, KickTraceDistance);
		const FVector LeftCone = PushForward.RotateAngleAxis(-KickHalfAngleDegrees, FVector::UpVector);
		const FVector RightCone = PushForward.RotateAngleAxis(KickHalfAngleDegrees, FVector::UpVector);
		DrawDebugLine(GetWorld(), Start, Start + LeftCone * ConeLength, FColor::Blue, false, 1.25f, 0, 1.0f);
		DrawDebugLine(GetWorld(), Start, Start + RightCone * ConeLength, FColor::Blue, false, 1.25f, 0, 1.0f);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("GameplayKickAOE executed=true TraceStart=%s TraceEnd=%s KickRadius=%.1f KickRange=%.1f OverlapCandidates=%d ValidKickTargets=%d TargetDetails=[%s] SelectedEnemy=%s Dot=%.3f Distance=%.1f KickResult=%s KnockbackAppliedCount=%d StunAppliedCount=%d EnemyAttacksCancelledCount=%d NextAllowedKickTime=%.3f"),
		*Start.ToCompactString(),
		*End.ToCompactString(),
		KickRadius,
		KickRange,
		CandidateCount,
		HitEnemyCount,
		*TargetDebugText,
		*GetNameSafe(SelectedEnemy),
		SelectedDot,
		SelectedDistance == TNumericLimits<float>::Max() ? -1.0f : SelectedDistance,
		GetKickResultName(bHit ? EOBKickResult::Hit : EOBKickResult::Miss),
		HitEnemyCount,
		HitEnemyCount,
		HitEnemyCount,
		NextAllowedKickTime);

	return bHit ? EOBKickResult::Hit : EOBKickResult::Miss;
}

bool AOBCharacter::IsEnemyInKickZone(const AOBEnemy* Enemy, const FVector& Start, const FVector& Forward, float MinForwardDot, float& OutDistance, float& OutDot) const
{
	OutDistance = -1.0f;
	OutDot = -1.0f;
	if (!Enemy || Enemy->IsDead())
	{
		return false;
	}

	const FVector ToEnemy = Enemy->GetActorLocation() - Start;
	const FVector ToEnemy2D = ToEnemy.GetSafeNormal2D();
	if (ToEnemy2D.IsNearlyZero())
	{
		OutDistance = 0.0f;
		OutDot = 1.0f;
		return true;
	}

	OutDistance = FVector::Dist2D(Start, Enemy->GetActorLocation());
	OutDot = FVector::DotProduct(Forward.GetSafeNormal2D(), ToEnemy2D);
	const float EffectiveKickRange = FMath::Max(KickRange, KickTraceDistance);
	const float EffectiveKickRadius = FMath::Max(KickRadius, KickTraceRadius);
	const float EffectiveCloseRadius = FMath::Max(KickEmergencyCloseRadius, EffectiveKickRadius * 0.5f);
	const float EnemyRadius = Enemy->GetCapsuleComponent() ? Enemy->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
	const bool bVeryClose = OutDistance <= EffectiveCloseRadius + EnemyRadius;
	const bool bInsideReach = OutDistance <= EffectiveKickRange + EnemyRadius;
	const bool bInsideAngle = OutDot >= (bVeryClose ? 0.0f : MinForwardDot);
	return bInsideReach && bInsideAngle;
}

TArray<AOBEnemy*> AOBCharacter::GatherKickTargets(FVector& OutStart, FVector& OutEnd, FVector& OutForward, FVector& OutOverlapCenter, float& OutOverlapRadius, int32& OutCandidateCount, FString& OutTargetDebugText) const
{
	TArray<AOBEnemy*> Targets;
	OutCandidateCount = 0;
	OutTargetDebugText.Reset();

	const UCameraComponent* KickCamera = FirstPersonCamera;
	OutForward = KickCamera ? KickCamera->GetForwardVector().GetSafeNormal2D() : GetActorForwardVector().GetSafeNormal2D();
	if (OutForward.IsNearlyZero())
	{
		OutForward = FVector::ForwardVector;
	}

	const FVector CameraLocation = KickCamera ? KickCamera->GetComponentLocation() : GetActorLocation() + FVector::UpVector * 45.0f;
	const float EffectiveKickRange = FMath::Max(KickRange, KickTraceDistance);
	const float EffectiveKickRadius = FMath::Max(KickRadius, KickTraceRadius);
	const float EffectiveHalfAngle = FMath::Max(KickHalfAngleDegrees, FMath::Clamp(KickConeAngleDegrees, 1.0f, 180.0f) * 0.5f);
	const float MinForwardDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(EffectiveHalfAngle, 1.0f, 90.0f)));

	OutStart = CameraLocation - OutForward * 30.0f - FVector::UpVector * 30.0f;
	OutEnd = OutStart + OutForward * EffectiveKickRange;
	OutOverlapCenter = CameraLocation + OutForward * (EffectiveKickRange * 0.5f);
	OutOverlapRadius = FMath::Max(EffectiveKickRange, EffectiveKickRadius);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(OneBulletKickGather), false, this);
	if (GetWorld())
	{
		GetWorld()->OverlapMultiByChannel(
			Overlaps,
			OutOverlapCenter,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeSphere(OutOverlapRadius),
			Params);
	}

	TArray<FString> TargetDebugParts;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AOBEnemy* Enemy = Cast<AOBEnemy>(Overlap.GetActor());
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}

		++OutCandidateCount;
		float Distance = -1.0f;
		float Dot = -1.0f;
		const bool bValidTarget = IsEnemyInKickZone(Enemy, OutStart, OutForward, MinForwardDot, Distance, Dot);
		const float AngleDegrees = Dot >= -1.0f ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f))) : -1.0f;
		TargetDebugParts.Add(FString::Printf(
			TEXT("%s dist=%.1f angle=%.1f valid=%s"),
			*GetNameSafe(Enemy),
			Distance,
			AngleDegrees,
			bValidTarget ? TEXT("true") : TEXT("false")));

		if (bKickDebugDraw && GetWorld())
		{
			const FColor CandidateColor = bValidTarget ? FColor::Green : FColor::Yellow;
			DrawDebugLine(GetWorld(), OutStart, Enemy->GetActorLocation(), CandidateColor, false, 1.25f, 0, 1.0f);
			DrawDebugSphere(GetWorld(), Enemy->GetActorLocation(), bValidTarget ? 28.0f : 20.0f, 8, CandidateColor, false, 1.25f);
		}

		if (bValidTarget)
		{
			Targets.AddUnique(Enemy);
		}
	}

	OutTargetDebugText = FString::Join(TargetDebugParts, TEXT("; "));
	return Targets;
}

void AOBCharacter::Dodge()
{
	if (bDead || bKickRecovering)
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
	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		OneBulletMode->HandleBulletPickedUp(this);
	}
}

void AOBCharacter::ConfirmPickupFeedback(const FVector& PickupLocation)
{
	OnPlayerBulletRecovered(PickupLocation);
	ApplyFeelStop(PickupStopDuration);
}

void AOBCharacter::SetCameraWeaponRelativeTransform(const FTransform& NewTransform)
{
	CameraWeaponRelativeTransform = NewTransform;
	if (bAttachWeaponToCamera)
	{
		ApplyWeaponReadyTransform(true);
	}
}

void AOBCharacter::SetPlayerLegRelativeTransform(const FTransform& NewTransform)
{
	bOverrideKickLegTransformFromVariables = true;
	KickLegRelativeLocation = NewTransform.GetLocation();
	KickLegRelativeRotation = NewTransform.GetRotation().Rotator();
	KickLegRelativeScale = NewTransform.GetScale3D();
	if (Player_Leg)
	{
		Player_Leg->SetRelativeTransform(NewTransform);
		KickLegInitialRelativeTransform = Player_Leg->GetRelativeTransform();
	}
}

void AOBCharacter::SetPlayerLegRelativeLocation(const FVector& NewLocation)
{
	bOverrideKickLegTransformFromVariables = true;
	KickLegRelativeLocation = NewLocation;
	if (Player_Leg)
	{
		Player_Leg->SetRelativeLocation(KickLegRelativeLocation);
		KickLegInitialRelativeTransform = Player_Leg->GetRelativeTransform();
	}
}

void AOBCharacter::SetPlayerLegRelativeRotation(const FRotator& NewRotation)
{
	bOverrideKickLegTransformFromVariables = true;
	KickLegRelativeRotation = NewRotation;
	if (Player_Leg)
	{
		Player_Leg->SetRelativeRotation(KickLegRelativeRotation);
		KickLegInitialRelativeTransform = Player_Leg->GetRelativeTransform();
	}
}

void AOBCharacter::SetPlayerLegRelativeScale(const FVector& NewScale)
{
	bOverrideKickLegTransformFromVariables = true;
	KickLegRelativeScale = NewScale;
	if (Player_Leg)
	{
		Player_Leg->SetRelativeScale3D(KickLegRelativeScale);
		KickLegInitialRelativeTransform = Player_Leg->GetRelativeTransform();
	}
}

FTransform AOBCharacter::GetPlayerLegRelativeTransform() const
{
	return Player_Leg ? Player_Leg->GetRelativeTransform() : FTransform(KickLegRelativeRotation, KickLegRelativeLocation, KickLegRelativeScale);
}

bool AOBCharacter::DidRecentKickProtectFromEnemy(const AOBEnemy* Enemy) const
{
	if (!Enemy || !GetWorld())
	{
		return false;
	}

	const float TimeSinceKick = GetWorld()->GetTimeSeconds() - LastSuccessfulKickTime;
	const float ProtectionWindow = FMath::Max(SuccessfulKickTouchKillGraceTime, KickAttackCancelWindow);
	if (TimeSinceKick < 0.0f || TimeSinceKick > ProtectionWindow)
	{
		return false;
	}

	if (LastSuccessfulKickEnemy.IsValid() && LastSuccessfulKickEnemy.Get() == Enemy)
	{
		return true;
	}

	const UCameraComponent* KickCamera = FirstPersonCamera;
	FVector Forward = KickCamera ? KickCamera->GetForwardVector().GetSafeNormal2D() : GetActorForwardVector().GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}

	const FVector CameraLocation = KickCamera ? KickCamera->GetComponentLocation() : GetActorLocation() + FVector::UpVector * 45.0f;
	const FVector Start = CameraLocation - Forward * 30.0f - FVector::UpVector * 30.0f;
	const float EffectiveHalfAngle = FMath::Max(KickHalfAngleDegrees, FMath::Clamp(KickConeAngleDegrees, 1.0f, 180.0f) * 0.5f);
	const float MinForwardDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(EffectiveHalfAngle, 1.0f, 90.0f)));
	float Distance = 0.0f;
	float Dot = -1.0f;
	return IsEnemyInKickZone(Enemy, Start, Forward, MinForwardDot, Distance, Dot);
}

void AOBCharacter::ToggleImmortalMode()
{
	bImmortalMode = !bImmortalMode;
	if (!bImmortalMode)
	{
		ActiveImmortalModeThreatSources.Empty();
		if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (AOBHUD* OneBulletHUD = Cast<AOBHUD>(PlayerController->GetHUD()))
			{
				OneBulletHUD->HideImmortalModeMsg();
			}
		}
	}
	OnPlayerImmortalModeChanged(bImmortalMode);
}

void AOBCharacter::TogglePerformanceDebug()
{
	SetPerformanceDebugEnabled(!bPerformanceDebugEnabled);
}

void AOBCharacter::SetPerformanceDebugEnabled(bool bEnabled)
{
	if (bPerformanceDebugEnabled == bEnabled)
	{
		if (bPerformanceDebugEnabled && GetWorld() && !GetWorldTimerManager().IsTimerActive(PerformanceDebugTimerHandle))
		{
			GetWorldTimerManager().SetTimer(
				PerformanceDebugTimerHandle,
				this,
				&AOBCharacter::UpdatePerformanceDebug,
				FMath::Max(PerformanceDebugLogInterval, 0.1f),
				true);
		}
		return;
	}

	bPerformanceDebugEnabled = bEnabled;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PerformanceDebugTimerHandle);
		if (bPerformanceDebugEnabled)
		{
			World->GetTimerManager().SetTimer(
				PerformanceDebugTimerHandle,
				this,
				&AOBCharacter::UpdatePerformanceDebug,
				FMath::Max(PerformanceDebugLogInterval, 0.1f),
				true);
		}
	}
	else
	{
		UE_LOG(LogOBPerformance, Warning, TEXT("Performance Debug changed before World was available; timer will not start yet."));
	}

	UE_LOG(
		LogOBPerformance,
		Log,
		TEXT("Performance Debug %s. Call DumpPerformanceSnapshot from Blueprint/C++ for an immediate snapshot. Suggested manual stats: stat unit, stat fps, stat game, stat gpu, stat anim, stat ai, stat audio."),
		bPerformanceDebugEnabled ? TEXT("enabled") : TEXT("disabled"));

	if (bPerformanceDebugEnabled)
	{
		DumpPerformanceSnapshot();
	}
}

void AOBCharacter::DumpPerformanceSnapshot()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogOBPerformance, Warning, TEXT("DumpPerformanceSnapshot called without a valid World; snapshot will contain fallback values."));
	}
	else
	{
		const AOBGameMode* OneBulletMode = World->GetAuthGameMode<AOBGameMode>();
		if (!OneBulletMode)
		{
			UE_LOG(LogOBPerformance, Warning, TEXT("DumpPerformanceSnapshot could not find AOBGameMode; WaveManager and PanicAudioManager counters may be zero."));
		}
		else
		{
			if (!OneBulletMode->GetWaveManager())
			{
				UE_LOG(LogOBPerformance, Warning, TEXT("DumpPerformanceSnapshot could not find WaveManager; wave counters will be zero."));
			}
			if (!OneBulletMode->PanicAudioManager)
			{
				UE_LOG(LogOBPerformance, Warning, TEXT("DumpPerformanceSnapshot could not find PanicAudioManager; panic audio counters will be zero."));
			}
		}
	}

	const FString Snapshot = BuildPerformanceSnapshotText();
	UE_LOG(LogOBPerformance, Warning, TEXT("%s"), *Snapshot);
}

void AOBCharacter::ResetForNewRun(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	bDead = false;
	bKickReady = true;
	bDodgeReady = true;
	bDodging = false;
	bKickRecovering = false;
	NextAllowedKickTime = 0.0f;
	bKickFOVPunchActive = false;
	bKickFOVReturning = false;
	bKickDashing = false;
	bKickWeaponSwayActive = false;
	ActiveImmortalModeThreatSources.Empty();
	ActiveDodgeDirection = FVector::ZeroVector;
	ActiveKickDashDirection = FVector::ZeroVector;
	ActiveDodgeElapsed = 0.0f;
	ActiveDodgePreviousAlpha = 0.0f;
	ActiveKickDashElapsed = 0.0f;
	ActiveKickDashPreviousAlpha = 0.0f;
	ActiveKickWeaponSwayElapsed = 0.0f;
	RemainingRecoilPitch = 0.0f;
	RemainingKickCameraTiltDown = 0.0f;
	SetWeaponBulletReady(true, true);
	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		OneBulletMode->StopPanicAudio(nullptr, false);
	}
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (AOBHUD* OneBulletHUD = Cast<AOBHUD>(PlayerController->GetHUD()))
		{
			OneBulletHUD->HideImmortalModeMsg();
		}
	}

	GetWorldTimerManager().ClearTimer(KickCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(KickRecoveryTimerHandle);
	GetWorldTimerManager().ClearTimer(KickLegAnimationTimerHandle);
	GetWorldTimerManager().ClearTimer(DodgeCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(ActionAnimationTimerHandle);
	FinishKickLegAnimation();
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
	if (bDead)
	{
		return;
	}

	if (bImmortalMode)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
		{
			if (AOBHUD* OneBulletHUD = Cast<AOBHUD>(PlayerController->GetHUD()))
			{
				OneBulletHUD->ShowImmortalModeMsg();
			}
		}
		return;
	}

	bDead = true;
	ActiveImmortalModeThreatSources.Empty();
	bKickRecovering = false;
	bKickDashing = false;
	bKickWeaponSwayActive = false;
	ActiveKickDashDirection = FVector::ZeroVector;
	ActiveKickDashElapsed = 0.0f;
	ActiveKickDashPreviousAlpha = 0.0f;
	ActiveKickWeaponSwayElapsed = 0.0f;
	RemainingKickCameraTiltDown = 0.0f;
	GetWorldTimerManager().ClearTimer(KickLegAnimationTimerHandle);
	FinishKickLegAnimation();
	GetWorldTimerManager().ClearTimer(KickRecoveryTimerHandle);
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

void AOBCharacter::ShowImmortalModeMsgForThreat(AActor* ThreatSource)
{
	if (bDead || !bImmortalMode)
	{
		return;
	}

	if (ThreatSource)
	{
		ActiveImmortalModeThreatSources.Add(ThreatSource);
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (AOBHUD* OneBulletHUD = Cast<AOBHUD>(PlayerController->GetHUD()))
		{
			OneBulletHUD->ShowImmortalModeMsg();
		}
	}
}

void AOBCharacter::HideImmortalModeMsgForThreat(AActor* ThreatSource)
{
	if (ThreatSource)
	{
		ActiveImmortalModeThreatSources.Remove(ThreatSource);
	}
	else
	{
		ActiveImmortalModeThreatSources.Empty();
	}

	for (auto It = ActiveImmortalModeThreatSources.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (!ActiveImmortalModeThreatSources.IsEmpty())
	{
		return;
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (AOBHUD* OneBulletHUD = Cast<AOBHUD>(PlayerController->GetHUD()))
		{
			OneBulletHUD->HideImmortalModeMsg();
		}
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

void AOBCharacter::FinishKickRecovery()
{
	bKickRecovering = false;
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

void AOBCharacter::UpdateKickDash(float DeltaSeconds)
{
	if (!bKickDashing)
	{
		return;
	}

	ActiveKickDashElapsed += DeltaSeconds;
	const float Duration = FMath::Max(KickDashDuration, 0.01f);
	const float RawAlpha = FMath::Clamp(ActiveKickDashElapsed / Duration, 0.0f, 1.0f);
	const float SmoothedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, RawAlpha, 2.0f);
	const float AlphaStep = SmoothedAlpha - ActiveKickDashPreviousAlpha;
	ActiveKickDashPreviousAlpha = SmoothedAlpha;

	if (!ActiveKickDashDirection.IsNearlyZero() && KickDashDistance > KINDA_SMALL_NUMBER && AlphaStep > 0.0f)
	{
		FHitResult DashHit;
		AddActorWorldOffset(ActiveKickDashDirection * KickDashDistance * AlphaStep, true, &DashHit);
	}

	if (RawAlpha >= 1.0f)
	{
		bKickDashing = false;
		ActiveKickDashDirection = FVector::ZeroVector;
		ActiveKickDashElapsed = 0.0f;
		ActiveKickDashPreviousAlpha = 0.0f;
	}
}

void AOBCharacter::UpdateKickCameraTilt(float DeltaSeconds)
{
	if (RemainingKickCameraTiltDown <= KINDA_SMALL_NUMBER || !Controller)
	{
		return;
	}

	const float Recovery = FMath::Min(RemainingKickCameraTiltDown, KickCameraTiltRecoverySpeed * DeltaSeconds);
	AddControllerPitchInput(-Recovery);
	RemainingKickCameraTiltDown -= Recovery;
}

void AOBCharacter::UpdateKickFOVPunch(float DeltaSeconds)
{
	if (!bKickFOVPunchActive && !bKickFOVReturning)
	{
		return;
	}

	KickFOVElapsed += DeltaSeconds;
	const bool bPunchingIn = bKickFOVPunchActive;
	const float Duration = FMath::Max(bPunchingIn ? KickFOVPunchInDuration : KickFOVPunchOutDuration, 0.01f);
	const float Alpha = FMath::Clamp(KickFOVElapsed / Duration, 0.0f, 1.0f);
	const float StartOffset = bPunchingIn ? 0.0f : KickFOVPunchAmount;
	const float TargetOffset = bPunchingIn ? KickFOVPunchAmount : 0.0f;
	const float CurrentOffset = FMath::Lerp(StartOffset, TargetOffset, Alpha);

	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(DefaultFirstPersonFOV + CurrentOffset);
	}

	if (Alpha < 1.0f)
	{
		return;
	}

	KickFOVElapsed = 0.0f;
	if (bKickFOVPunchActive)
	{
		bKickFOVPunchActive = false;
		bKickFOVReturning = true;
	}
	else
	{
		bKickFOVReturning = false;
	}
}

void AOBCharacter::StartKickFOVPunch()
{
	if (KickFOVPunchAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	KickFOVElapsed = 0.0f;
	bKickFOVPunchActive = true;
	bKickFOVReturning = false;
}

void AOBCharacter::StartKickDash(const FVector& Direction)
{
	ActiveKickDashDirection = Direction.GetSafeNormal2D();
	if (ActiveKickDashDirection.IsNearlyZero() || KickDashDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	bKickDashing = true;
	ActiveKickDashElapsed = 0.0f;
	ActiveKickDashPreviousAlpha = 0.0f;
}

void AOBCharacter::StartKickWeaponSway()
{
	bKickWeaponSwayActive = true;
	ActiveKickWeaponSwayElapsed = 0.0f;
}

void AOBCharacter::PlayKickStartFeedback()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (KickCameraShake && PlayerController->PlayerCameraManager)
		{
			PlayerController->PlayerCameraManager->StartCameraShake(KickCameraShake);
		}
	}

	StartKickFOVPunch();

	if (Controller && KickCameraTiltDownAmount > KINDA_SMALL_NUMBER)
	{
		AddControllerPitchInput(KickCameraTiltDownAmount);
		RemainingKickCameraTiltDown += KickCameraTiltDownAmount;
	}
}

void AOBCharacter::PlayKickImpactFeedback(const FVector& Origin, const FVector& Direction, int32 HitEnemyCount)
{
	const FVector SafeDirection = Direction.GetSafeNormal2D();
	const FVector EffectLocation = Origin + SafeDirection * 85.0f - FVector::UpVector * 35.0f;

	if (HitEnemyCount <= 0)
	{
		if (KickWhooshSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, KickWhooshSound, GetActorLocation());
		}
		else if (KickSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, KickSound, GetActorLocation());
		}
		return;
	}

	if (KickImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, KickImpactSound, EffectLocation);
	}
	else if (KickSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, KickSound, EffectLocation);
	}

	UParticleSystem* ImpactEffect = KickImpactVFX ? KickImpactVFX.Get() : KickPushEffect.Get();
	if (ImpactEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactEffect,
			FTransform(SafeDirection.Rotation(), EffectLocation),
			true,
			EPSCPoolMethod::AutoRelease);
	}

	if (Controller)
	{
		if (KickImpactCameraPitchPunch > KINDA_SMALL_NUMBER)
		{
			AddControllerPitchInput(KickImpactCameraPitchPunch);
			RemainingKickCameraTiltDown += KickImpactCameraPitchPunch;
		}
		if (KickImpactCameraYawPunch > KINDA_SMALL_NUMBER)
		{
			AddControllerYawInput(FMath::FRandRange(-KickImpactCameraYawPunch, KickImpactCameraYawPunch));
		}
	}
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
		const float RequiredClearance = DodgeEnemyClearance + Enemy->GetEffectiveAttackRadius();

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
	GetMesh()->bCastHiddenShadow = false;
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	GetMesh()->SetRelativeScale3D(FVector(1.0f));
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->SetHiddenInGame(true);
	GetMesh()->SetVisibility(false, true);

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

	if (!KickAnimation)
	{
		static ConstructorHelpers::FObjectFinder<UAnimationAsset> DefaultKickAnimation(TEXT("/Game/Assets/Animations/KickingLeg_Anim.KickingLeg_Anim"));
		if (DefaultKickAnimation.Succeeded())
		{
			KickAnimation = DefaultKickAnimation.Object;
		}
	}
}

void AOBCharacter::ConfigureFirstPersonBodyVisibility()
{
	if (!GetMesh())
	{
		return;
	}

	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->SetHiddenInGame(true);
	GetMesh()->SetVisibility(false, true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AOBCharacter::ConfigureKickLeg()
{
	if (!Player_Leg)
	{
		return;
	}

	if (FirstPersonCamera && Player_Leg->GetAttachParent() != FirstPersonCamera)
	{
		Player_Leg->AttachToComponent(FirstPersonCamera, FAttachmentTransformRules::KeepRelativeTransform);
	}

	if (bOverrideKickLegTransformFromVariables)
	{
		Player_Leg->SetRelativeLocation(KickLegRelativeLocation);
		Player_Leg->SetRelativeRotation(KickLegRelativeRotation);
		Player_Leg->SetRelativeScale3D(KickLegRelativeScale);
	}
	Player_Leg->BoundsScale = 10.0f;
	Player_Leg->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Player_Leg->PrimaryComponentTick.SetTickFunctionEnable(true);
	Player_Leg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Player_Leg->SetOnlyOwnerSee(true);
	Player_Leg->SetOwnerNoSee(false);
	Player_Leg->SetCastShadow(false);
	Player_Leg->bCastHiddenShadow = false;
	const bool bIsGameWorld = GetWorld() && GetWorld()->IsGameWorld();
	Player_Leg->SetHiddenInGame(bIsGameWorld && !bKickVisualCalibrationMode);
	Player_Leg->SetVisibility(!bIsGameWorld || bKickVisualCalibrationMode, true);

	if (KickLegMesh)
	{
		Player_Leg->SetSkeletalMesh(KickLegMesh);
	}
	else if (bUseFullBodyMeshAsKickSource && GetMesh() && GetMesh()->GetSkeletalMeshAsset())
	{
		Player_Leg->SetSkeletalMesh(GetMesh()->GetSkeletalMeshAsset());
	}
	else if (!Player_Leg->GetSkeletalMeshAsset())
	{
		UE_LOG(LogOBKickVisual, Warning, TEXT("Player_Leg is configured but no first-person leg SkeletalMesh is assigned. Set KickLegMesh or the Player_Leg component mesh in Blueprint."));
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
	WeaponMesh->SetCastShadow(false);
	WeaponMesh->bCastHiddenShadow = false;
	if (WeaponModel)
	{
		WeaponMesh->SetSkeletalMesh(WeaponModel);
	}
}

void AOBCharacter::ApplyWeaponReadyTransform(bool bSnap)
{
	if (!WeaponMesh || !bWeaponBulletReady)
	{
		bKickWeaponSwayActive = false;
		ActiveKickWeaponSwayElapsed = 0.0f;
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

	FTransform TargetTransform = GetWeaponReadyTargetTransform();
	if (bKickWeaponSwayActive)
	{
		ActiveKickWeaponSwayElapsed += DeltaSeconds;
		const float Duration = FMath::Max(KickWeaponSwayDuration, 0.01f);
		const float RawAlpha = FMath::Clamp(ActiveKickWeaponSwayElapsed / Duration, 0.0f, 1.0f);
		const float SwayAlpha = FMath::Sin(RawAlpha * PI);
		TargetTransform.AddToTranslation(FVector(-KickWeaponSwayBack, 0.0f, -KickWeaponSwayDown) * SwayAlpha);
		TargetTransform.ConcatenateRotation(FRotator(-KickWeaponSwayPitch * SwayAlpha, 0.0f, 0.0f).Quaternion());
		if (RawAlpha >= 1.0f)
		{
			bKickWeaponSwayActive = false;
			ActiveKickWeaponSwayElapsed = 0.0f;
		}
	}

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

bool AOBCharacter::ApplyKickLegOnlyMask()
{
	if (!Player_Leg)
	{
		return false;
	}

	LastKickMaskHiddenBoneNames.Reset();
	LastKickMaskHiddenBoneList.Reset();
	LastKickMaskVisibleWhitelist.Reset();

	USkeletalMesh* CurrentLegMesh = Player_Leg->GetSkeletalMeshAsset();
	if (!CurrentLegMesh)
	{
		UE_LOG(LogOBKickVisual, Warning, TEXT("ApplyKickLegOnlyMask called but Player_Leg has no skeletal mesh."));
		return false;
	}

	const TArray<FName> KickingLegBones = bKickUsesRightLeg
		? TArray<FName>{ TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r") }
		: TArray<FName>{ TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("ball_l") };

	TSet<FName> VisibleBoneWhitelist;
	VisibleBoneWhitelist.Add(TEXT("root"));
	VisibleBoneWhitelist.Add(TEXT("pelvis"));
	for (const FName& BoneName : KickingLegBones)
	{
		VisibleBoneWhitelist.Add(BoneName);
	}

	LastKickMaskVisibleWhitelist = FString::JoinBy(
		VisibleBoneWhitelist,
		TEXT(", "),
		[](const FName& Name)
		{
			return Name.ToString();
		});

	const FReferenceSkeleton& ReferenceSkeleton = CurrentLegMesh->GetRefSkeleton();
	for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
	{
		const FName BoneName = ReferenceSkeleton.GetBoneName(BoneIndex);
		if (BoneName.IsNone() || VisibleBoneWhitelist.Contains(BoneName))
		{
			continue;
		}

		Player_Leg->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
		LastKickMaskHiddenBoneNames.Add(BoneName);
	}

	LastKickMaskHiddenBoneList = FString::JoinBy(
		LastKickMaskHiddenBoneNames,
		TEXT(", "),
		[](const FName& Name)
		{
			return Name.ToString();
		});
	KickVisibleBoneNames = KickingLegBones;
	KickHiddenBoneNames = LastKickMaskHiddenBoneNames;

	UE_LOG(
		LogOBKickVisual,
		Log,
		TEXT("ApplyKickLegOnlyMask called. KickUsesRightLeg=%s Hidden bones count=%d Hidden bones list=%s Visible leg whitelist=%s KickVisualHideEarlyTime=%.3f"),
		bKickUsesRightLeg ? TEXT("true") : TEXT("false"),
		LastKickMaskHiddenBoneNames.Num(),
		*LastKickMaskHiddenBoneList,
		*LastKickMaskVisibleWhitelist,
		KickVisualHideEarlyTime);

	ShowKickVisualDebugMessage(FString::Printf(TEXT("Kick mask applied: %s leg only"), bKickUsesRightLeg ? TEXT("right") : TEXT("left")));
	ShowKickVisualDebugMessage(FString::Printf(TEXT("Hidden bones: %d"), LastKickMaskHiddenBoneNames.Num()));
	ShowKickVisualDebugMessage(FString::Printf(TEXT("Hide early: %.2fs"), KickVisualHideEarlyTime));

	return LastKickMaskHiddenBoneNames.Num() > 0;
}

void AOBCharacter::ResetKickBoneMask()
{
	if (!Player_Leg)
	{
		return;
	}

	for (const FName& BoneName : LastKickMaskHiddenBoneNames)
	{
		if (!BoneName.IsNone() && Player_Leg->GetBoneIndex(BoneName) != INDEX_NONE)
		{
			Player_Leg->UnHideBoneByName(BoneName);
		}
	}
	LastKickMaskHiddenBoneNames.Reset();
	LastKickMaskHiddenBoneList.Reset();
}

void AOBCharacter::ShowKickVisualDebugMessage(const FString& Message) const
{
	if (bKickVisualDebug)
	{
		UE_LOG(LogOBKickVisual, Log, TEXT("%s"), *Message);
	}
}

bool AOBCharacter::PlayKickLegAnimation()
{
	UE_LOG(LogOBKickVisual, Log, TEXT("Kick input pressed"));
	if (!Player_Leg)
	{
		UE_LOG(LogOBKickVisual, Warning, TEXT("Kick input pressed but Player_Leg component was not found on %s."), *GetName());
		ShowKickVisualDebugMessage(TEXT("Kick anim not visible: no Player_Leg component"));
		return false;
	}

	const FTransform RelativeTransformBeforeConfigure = Player_Leg->GetRelativeTransform();
	ConfigureKickLeg();
	const FTransform RelativeTransformAfterConfigure = Player_Leg->GetRelativeTransform();
	UE_LOG(LogOBKickVisual, Log, TEXT("Kick input: Player_Leg component found: %s."), *GetNameSafe(Player_Leg));

	USkeletalMesh* CurrentLegMesh = Player_Leg->GetSkeletalMeshAsset();
	if (!CurrentLegMesh)
	{
		UE_LOG(LogOBKickVisual, Warning, TEXT("Kick input pressed but Player_Leg has no first-person leg SkeletalMesh assigned. Set KickLegMesh or Player_Leg mesh in Blueprint."));
		ShowKickVisualDebugMessage(TEXT("Kick anim not visible: no skeletal mesh"));
		return false;
	}
	UE_LOG(LogOBKickVisual, Log, TEXT("Kick input: Player_Leg SkeletalMesh assigned: %s."), *GetNameSafe(CurrentLegMesh));
	if (IsDisallowedKickLegMesh(CurrentLegMesh))
	{
		if (!bUseFullBodyMeshAsKickSource)
		{
			UE_LOG(LogOBKickVisual, Warning, TEXT("Player_Leg uses full-body mesh and bUseFullBodyMeshAsKickSource is disabled. Current mesh: %s"), *CurrentLegMesh->GetPathName());
			ShowKickVisualDebugMessage(TEXT("Kick anim not visible: full-body mesh blocked"));
			return false;
		}
		UE_LOG(LogOBKickVisual, Warning, TEXT("Player_Leg uses full-body mesh as animation source. Applying first-person kick bone mask. Current mesh: %s"), *CurrentLegMesh->GetPathName());
	}

	if (!KickAnimation)
	{
		UE_LOG(LogOBKickVisual, Warning, TEXT("Kick input pressed but KickingLeg_Anim / KickAnimation is not assigned."));
		ShowKickVisualDebugMessage(TEXT("Kick anim not visible: anim not assigned"));
		return false;
	}
	UE_LOG(LogOBKickVisual, Log, TEXT("Kick input: KickingLeg_Anim assigned: %s."), *GetNameSafe(KickAnimation));

	const USkeleton* LegSkeleton = CurrentLegMesh->GetSkeleton();
	const USkeleton* AnimSkeleton = KickAnimation->GetSkeleton();
	const bool bSkeletonCompatible = IsAnimationCompatibleWithMesh(KickAnimation, Player_Leg);
	UE_LOG(
		LogOBKickVisual,
		Log,
		TEXT("Kick input: Player_Leg skeleton=%s KickingLeg_Anim skeleton=%s Skeleton compatible=%s."),
		*GetNameSafe(LegSkeleton),
		*GetNameSafe(AnimSkeleton),
		bSkeletonCompatible ? TEXT("true") : TEXT("false"));

	if (!bSkeletonCompatible)
	{
		UE_LOG(LogOBKickVisual, Warning, TEXT("Player_Leg mesh skeleton does not match KickingLeg_Anim skeleton. Kick visual skipped because assets are invalid."));
		ShowKickVisualDebugMessage(TEXT("Kick anim not visible: skeleton mismatch"));
		return false;
	}

	UE_LOG(
		LogOBKickVisual,
		Log,
		TEXT("Kick input: bOverrideKickLegTransformFromVariables=%s KickLegRelativeLocation=%s KickLegRelativeRotation=%s KickLegRelativeScale=%s"),
		bOverrideKickLegTransformFromVariables ? TEXT("true") : TEXT("false"),
		*KickLegRelativeLocation.ToCompactString(),
		*KickLegRelativeRotation.ToCompactString(),
		*KickLegRelativeScale.ToCompactString());
	UE_LOG(
		LogOBKickVisual,
		Log,
		TEXT("Kick input: Player_Leg=%s Parent=%s Mesh=%s Anim=%s RelativeBeforeConfigure=%s RelativeAfterConfigure=%s InitialRelativeTransform=%s WorldTransform=%s"),
		*GetNameSafe(Player_Leg),
		*GetNameSafe(Player_Leg->GetAttachParent()),
		*GetNameSafe(CurrentLegMesh),
		*GetNameSafe(KickAnimation),
		*RelativeTransformBeforeConfigure.ToHumanReadableString(),
		*RelativeTransformAfterConfigure.ToHumanReadableString(),
		*KickLegInitialRelativeTransform.ToHumanReadableString(),
		*Player_Leg->GetComponentTransform().ToHumanReadableString());

	const bool bWasHiddenBeforeShow = Player_Leg->bHiddenInGame;
	Player_Leg->SetHiddenInGame(false);
	Player_Leg->SetVisibility(true, true);
	Player_Leg->SetComponentTickEnabled(true);
	Player_Leg->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	const bool bBoneMaskApplied = ApplyKickLegOnlyMask();
	UE_LOG(LogOBKickVisual, Log, TEXT("Kick visual: Player_Leg hidden before show=%s visible after show=%s Bone mask applied=%s Hidden bones count=%d Visible leg whitelist=%s"),
		bWasHiddenBeforeShow ? TEXT("true") : TEXT("false"),
		Player_Leg->IsVisible() && !Player_Leg->bHiddenInGame ? TEXT("true") : TEXT("false"),
		bBoneMaskApplied ? TEXT("true") : TEXT("false"),
		LastKickMaskHiddenBoneNames.Num(),
		*LastKickMaskVisibleWhitelist);

	if (UAnimMontage* KickMontage = Cast<UAnimMontage>(KickAnimation))
	{
		Player_Leg->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		if (UAnimInstance* LegAnimInstance = Player_Leg->GetAnimInstance())
		{
			const float PlayedDuration = LegAnimInstance->Montage_Play(KickMontage, FMath::Max(KickPlayRate, 0.01f));
			if (PlayedDuration > 0.0f)
			{
				UE_LOG(LogOBKickVisual, Log, TEXT("Started first-person kick montage %s on Player_Leg."), *GetNameSafe(KickMontage));
				GetWorldTimerManager().ClearTimer(KickLegAnimationTimerHandle);
				const float HideDelay = FMath::Max(0.01f, PlayedDuration - FMath::Max(KickVisualHideEarlyTime, 0.0f));
				if (!bKickVisualCalibrationMode)
				{
					GetWorldTimerManager().SetTimer(
						KickLegAnimationTimerHandle,
						this,
						&AOBCharacter::FinishKickLegAnimation,
						HideDelay,
						false);
				}
				UE_LOG(LogOBKickVisual, Log, TEXT("Kick visual: Animation mode=Montage Animation length=%.2f KickVisualHideEarlyTime=%.2f Actual hide delay=%.2f Animation started on Player_Leg=true Player_Leg will hide after %.2f seconds"),
					PlayedDuration,
					KickVisualHideEarlyTime,
					bKickVisualCalibrationMode ? -1.0f : HideDelay,
					bKickVisualCalibrationMode ? -1.0f : HideDelay);
			}
			else
			{
				UE_LOG(LogOBKickVisual, Warning, TEXT("Failed to start first-person kick montage %s on Player_Leg."), *GetNameSafe(KickMontage));
				ShowKickVisualDebugMessage(TEXT("Kick anim not visible: anim not started"));
				return false;
			}
		}
		else
		{
			UE_LOG(LogOBKickVisual, Warning, TEXT("KickingLeg_Anim is a montage, but Player_Leg has no AnimInstance to play it."));
			ShowKickVisualDebugMessage(TEXT("Kick anim not visible: no Player_Leg AnimInstance"));
			return false;
		}
		return true;
	}

	Player_Leg->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Player_Leg->PlayAnimation(KickAnimation, bKickVisualCalibrationMode);
	if (UAnimSingleNodeInstance* SingleNodeInstance = Player_Leg->GetSingleNodeInstance())
	{
		SingleNodeInstance->SetPlayRate(FMath::Max(bKickVisualCalibrationMode ? KickVisualCalibrationPlayRate : KickPlayRate, 0.01f));
	}

	UE_LOG(LogOBKickVisual, Log, TEXT("Started first-person kick animation %s on Player_Leg."), *GetNameSafe(KickAnimation));

	const float BaseDuration = FMath::Max(KickAnimationDuration, KickAnimation->GetPlayLength());
	const float ScaledDuration = BaseDuration / FMath::Max(bKickVisualCalibrationMode ? KickVisualCalibrationPlayRate : KickPlayRate, 0.01f);
	const float HideDelay = FMath::Max(0.01f, ScaledDuration - FMath::Max(KickVisualHideEarlyTime, 0.0f));
	GetWorldTimerManager().ClearTimer(KickLegAnimationTimerHandle);
	if (!bKickVisualCalibrationMode)
	{
		GetWorldTimerManager().SetTimer(
			KickLegAnimationTimerHandle,
			this,
			&AOBCharacter::FinishKickLegAnimation,
			HideDelay,
			false);
	}
	UE_LOG(LogOBKickVisual, Log, TEXT("Kick visual: Animation mode=SingleNode Animation length=%.2f KickVisualHideEarlyTime=%.2f Actual hide delay=%.2f Animation started on Player_Leg=true Player_Leg will hide after %.2f seconds"),
		ScaledDuration,
		KickVisualHideEarlyTime,
		bKickVisualCalibrationMode ? -1.0f : HideDelay,
		bKickVisualCalibrationMode ? -1.0f : HideDelay);
	return true;
}

void AOBCharacter::FinishKickLegAnimation()
{
	if (Player_Leg)
	{
		if (bKickVisualCalibrationMode)
		{
			ApplyKickLegOnlyMask();
			Player_Leg->SetHiddenInGame(false);
			Player_Leg->SetVisibility(true, true);
			return;
		}
		Player_Leg->SetHiddenInGame(true);
		Player_Leg->SetVisibility(false, true);
		UE_LOG(
			LogOBKickVisual,
			Log,
			TEXT("Kick visual: Player_Leg hidden before ref pose reset: %s"),
			Player_Leg->bHiddenInGame ? TEXT("true") : TEXT("false"));
		ResetKickBoneMask();
	}
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

void AOBCharacter::PlayActionAnimation(UAnimationAsset* Animation, float Duration, float PlayRate, bool bUseFullAnimationLength)
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
	if (UAnimSingleNodeInstance* SingleNodeInstance = GetMesh()->GetSingleNodeInstance())
	{
		SingleNodeInstance->SetPlayRate(FMath::Max(PlayRate, 0.01f));
	}
	if (FullBodyShadowMesh)
	{
		FullBodyShadowMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		FullBodyShadowMesh->PlayAnimation(Animation, false);
		if (UAnimSingleNodeInstance* ShadowSingleNodeInstance = FullBodyShadowMesh->GetSingleNodeInstance())
		{
			ShadowSingleNodeInstance->SetPlayRate(FMath::Max(PlayRate, 0.01f));
		}
	}
	const float BaseDuration = bUseFullAnimationLength ? FMath::Max(Duration, Animation->GetPlayLength()) : Duration;
	const float ScaledDuration = BaseDuration / FMath::Max(PlayRate, 0.01f);
	GetWorldTimerManager().SetTimer(ActionAnimationTimerHandle, this, &AOBCharacter::RestoreMovementAnimation, FMath::Max(ScaledDuration, 0.01f), false);
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

void AOBCharacter::LoadMouseSensitivity()
{
	float SavedSensitivity = MouseSensitivity;
	if (GConfig)
	{
		GConfig->GetFloat(MouseSettingsSection, MouseSensitivityKey, SavedSensitivity, GGameUserSettingsIni);
	}

	MouseSensitivity = GetClampedMouseSensitivity(SavedSensitivity);
}

void AOBCharacter::SaveMouseSensitivity() const
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetFloat(MouseSettingsSection, MouseSensitivityKey, MouseSensitivity, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

float AOBCharacter::GetClampedMouseSensitivity(float Value) const
{
	const float MinSensitivity = FMath::Max(MinMouseSensitivity, 0.01f);
	const float MaxSensitivity = FMath::Max(MaxMouseSensitivity, MinSensitivity);
	const float RoundedValue = FMath::RoundToFloat(Value * MouseSensitivityDisplayPrecision) / MouseSensitivityDisplayPrecision;
	return FMath::Clamp(RoundedValue, MinSensitivity, MaxSensitivity);
}

void AOBCharacter::GetCrosshairTrace(FVector& OutTraceStart, FVector& OutTraceEnd) const
{
	OutTraceStart = FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : GetActorLocation();
	FVector AimDirection = FirstPersonCamera ? FirstPersonCamera->GetForwardVector() : GetActorForwardVector();

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

void AOBCharacter::UpdatePerformanceDebug()
{
	if (!bPerformanceDebugEnabled)
	{
		return;
	}

	const FString Snapshot = BuildPerformanceSnapshotText();
	UE_LOG(LogOBPerformance, Log, TEXT("%s"), *Snapshot);
}

FString AOBCharacter::BuildPerformanceSnapshotText() const
{
	const UWorld* World = GetWorld();
	const float FrameSeconds = World ? World->GetDeltaSeconds() : FApp::GetDeltaTime();
	const float FrameMs = FrameSeconds * 1000.0f;
	const float FPS = FrameSeconds > SMALL_NUMBER ? 1.0f / FrameSeconds : 0.0f;

	int32 AliveEnemies = 0;
	int32 TotalEnemyActors = 0;
	int32 FastEnemies = 0;
	int32 HeavyEnemies = 0;
	int32 ActiveAIControllers = 0;
	int32 BulletPickups = 0;
	int32 BulletTrails = 0;
	int32 VFXOrDebugActors = 0;
	int32 ActiveWorldAudioComponents = 0;

	if (World)
	{
		for (TActorIterator<AOBEnemy> It(World); It; ++It)
		{
			++TotalEnemyActors;
			if (!It->IsDead())
			{
				++AliveEnemies;
				if (It->EnemyType == EOBEnemyType::Heavy)
				{
					++HeavyEnemies;
				}
				else
				{
					++FastEnemies;
				}
			}
		}

		for (TActorIterator<AAIController> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetPawn())
			{
				++ActiveAIControllers;
			}
		}

		for (TActorIterator<AOBBulletPickup> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				++BulletPickups;
			}
		}

		for (TActorIterator<AOBBulletTrailActor> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				++BulletTrails;
			}
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const AActor* Actor = *It;
			if (!IsValid(Actor))
			{
				continue;
			}

			TArray<UAudioComponent*> AudioComponents;
			Actor->GetComponents<UAudioComponent>(AudioComponents);
			for (const UAudioComponent* Component : AudioComponents)
			{
				if (Component && Component->IsPlaying())
				{
					++ActiveWorldAudioComponents;
				}
			}

			const FString ClassName = Actor->GetClass() ? Actor->GetClass()->GetName() : FString();
			if (ClassName.Contains(TEXT("Niagara"))
				|| ClassName.Contains(TEXT("Debug"))
				|| ClassName.Contains(TEXT("TextRender"))
				|| ClassName.Contains(TEXT("PointLight"))
				|| Actor->IsA<AOBBulletTrailActor>())
			{
				++VFXOrDebugActors;
			}
		}
	}

	int32 VisibleWidgets = 0;
	int32 TotalWidgets = 0;
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		const UUserWidget* Widget = *It;
		if (!Widget || Widget->GetWorld() != World)
		{
			continue;
		}

		++TotalWidgets;
		if (IsVisibleUserWidget(Widget))
		{
			++VisibleWidgets;
		}
	}

	const AOBGameMode* OneBulletMode = World ? World->GetAuthGameMode<AOBGameMode>() : nullptr;
	const AOBWaveManager* WaveManager = OneBulletMode ? OneBulletMode->GetWaveManager() : nullptr;
	const AOBPanicAudioManager* PanicAudio = OneBulletMode ? OneBulletMode->PanicAudioManager.Get() : nullptr;
	const AOBGameState* OneBulletState = World ? World->GetGameState<AOBGameState>() : nullptr;

	const int32 PanicAudioComponents = PanicAudio ? PanicAudio->GetActivePanicAudioComponentCount() : 0;
	const int32 HeartbeatComponents = PanicAudio ? PanicAudio->GetActiveHeartbeatComponentCount() : 0;
	const int32 RoarComponents = PanicAudio ? PanicAudio->GetActiveRoarComponentCount() : 0;
	const int32 FootstepComponents = PanicAudio ? PanicAudio->GetActiveFootstepComponentCount() : 0;
	const int32 MusicComponents = PanicAudio ? PanicAudio->GetActiveBackgroundMusicComponentCount() : 0;

	return FString::Printf(
		TEXT("[OB PERF] FPS=%.1f Frame=%.2fms | Player HasBullet=%s Panic=%s | Enemies alive=%d totalActors=%d fast=%d heavy=%d AIControllers=%d | Audio panicComponents=%d heartbeat=%d roar=%d footsteps=%d music=%d worldActiveAudio=%d | Widgets visible=%d total=%d | Wave current=%d alive=%d spawnedThisWave=%d pending=%d heavySpawnedThisWave=%d totalSpawned=%d tracked=%d | Actors pickups=%d bulletTrails=%d vfxDebugApprox=%d spawnWarningVFX=%d spawnWarningLights=%d"),
		FPS,
		FrameMs,
		OneBulletState && OneBulletState->HasBullet() ? TEXT("true") : TEXT("false"),
		PanicAudio && PanicAudio->IsPanicAudioActive() ? TEXT("true") : TEXT("false"),
		AliveEnemies,
		TotalEnemyActors,
		FastEnemies,
		HeavyEnemies,
		ActiveAIControllers,
		PanicAudioComponents,
		HeartbeatComponents,
		RoarComponents,
		FootstepComponents,
		MusicComponents,
		ActiveWorldAudioComponents,
		VisibleWidgets,
		TotalWidgets,
		WaveManager ? WaveManager->CurrentWaveNumber : 0,
		WaveManager ? WaveManager->LivingEnemyCount : 0,
		WaveManager ? WaveManager->SpawnedThisWave : 0,
		WaveManager ? WaveManager->GetPendingSpawnCount() : 0,
		WaveManager ? WaveManager->HeavyEnemiesSpawnedThisWave : 0,
		WaveManager ? WaveManager->TotalEnemiesSpawned : 0,
		WaveManager ? WaveManager->GetTrackedEnemyCount() : 0,
		BulletPickups,
		BulletTrails,
		VFXOrDebugActors,
		WaveManager ? WaveManager->SpawnWarningVFXSpawned : 0,
		WaveManager ? WaveManager->SpawnWarningLightsSpawned : 0);
}

AOBBulletPickup* AOBCharacter::DropBulletAt(const FVector& Location)
{
	if (AOBGameMode* OneBulletMode = GetWorld() ? GetWorld()->GetAuthGameMode<AOBGameMode>() : nullptr)
	{
		return OneBulletMode->SpawnBulletPickup(Location);
	}
	return nullptr;
}
