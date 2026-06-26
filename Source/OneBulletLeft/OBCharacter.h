#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "OBCharacter.generated.h"

class UCameraComponent;
class UCameraShakeBase;
class UAnimationAsset;
class UAnimInstance;
class UParticleSystem;
class USceneComponent;
class USpringArmComponent;
class USoundBase;
class USkeletalMesh;
class AOBBulletPickup;

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOBCharacter();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> ThirdPersonSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> FullBodyShadowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Visual")
	bool bHideHeadForFirstPerson = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Visual")
	bool bHideBodyForFirstPersonCameraWeapon = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|View")
	bool bStartInThirdPerson = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|View")
	float ThirdPersonCameraDistance = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|View")
	FVector ThirdPersonCameraOffset = FVector(0.0f, 40.0f, 62.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|View")
	bool bThirdPersonCameraLag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|View")
	float ThirdPersonCameraLagSpeed = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Controls", meta=(ClampMin="0.05", ClampMax="5.0", UIMin="0.1", UIMax="3.0"))
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Controls", meta=(ClampMin="0.01", UIMin="0.01", UIMax="1.0"))
	float MinMouseSensitivity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Controls", meta=(ClampMin="0.1", UIMin="1.0", UIMax="5.0"))
	float MaxMouseSensitivity = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Controls", meta=(ClampMin="0.01", UIMin="0.05", UIMax="0.5"))
	float MouseSensitivityStep = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Modes")
	bool bStartImmortal = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Shooting")
	float ShootRange = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Shooting|Feel")
	float RecoilPitchImpulse = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Shooting|Feel")
	float RecoilYawRandomness = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Shooting|Feel")
	float RecoilRecoverySpeed = 11.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Shooting|Feel")
	TSubclassOf<UCameraShakeBase> ShootCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Shooting|Feel")
	float HitStopDuration = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Shooting|Feel", meta=(ClampMin="0.01", ClampMax="1.0"))
	float HitStopTimeDilation = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pickup|Feel")
	float PickupStopDuration = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick")
	float KickRange = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick")
	float KickRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="1.0", ClampMax="180.0", UIMin="60.0", UIMax="90.0"))
	float KickConeAngleDegrees = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="500.0", UIMax="900.0"))
	float KickPushDistance = 760.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.01", UIMin="0.10", UIMax="0.35"))
	float KickPushDuration = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.2", UIMax="1.0"))
	float KickStunDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.5", UIMax="0.7"))
	float KickSlowMultiplier = 0.60f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.5", UIMax="1.0"))
	float KickSlowDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.05", UIMax="0.4"))
	float KickRecoveryDuration = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.0", UIMax="80.0"))
	float KickPlayerLungeDistance = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick")
	float KickCooldown = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel")
	TSubclassOf<UCameraShakeBase> KickCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="8.0"))
	float KickFOVPunchAmount = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.01", UIMin="0.05", UIMax="0.2"))
	float KickFOVPunchInDuration = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.01", UIMin="0.05", UIMax="0.3"))
	float KickFOVPunchOutDuration = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel")
	TObjectPtr<UParticleSystem> KickPushEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> KickAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	float KickAnimationDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> ShootAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	float ShootAnimationDuration = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> DeathAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(DisplayName="Standard Idle Animation (Bullet Lost)"))
	TObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(DisplayName="Pistol Idle Animation (Bullet Ready)"))
	TObjectPtr<UAnimationAsset> PistolIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(DisplayName="Standard Run Animation (Bullet Lost)"))
	TObjectPtr<UAnimationAsset> RunAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(DisplayName="Pistol Run Animation (Bullet Ready)"))
	TObjectPtr<UAnimationAsset> PistolRunAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	bool bUseSimpleLocomotionAnimations = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation", meta=(EditCondition="bUseSimpleLocomotionAnimations", ClampMin="0.0"))
	float RunAnimationMinSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Dodge", meta=(ClampMin="100.0", UIMin="300.0", UIMax="1400.0"))
	float DodgeDistance = 950.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Dodge", meta=(ClampMin="0.03", UIMin="0.05", UIMax="0.30"))
	float DodgeDuration = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Dodge", meta=(ClampMin="0.0", UIMin="0.0", UIMax="3.0"))
	float DodgeCooldown = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Dodge", meta=(ClampMin="0.0", UIMin="0.0", UIMax="400.0"))
	float DodgeEnemyClearance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Dodge")
	bool bPreferMovementDirectionDodge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Dodge")
	FKey DodgeKey = EKeys::LeftShift;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Dodge")
	FKey SecondaryDodgeKey = EKeys::RightShift;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon")
	TObjectPtr<USkeletalMesh> WeaponModel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon")
	FName WeaponAttachSocketName = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon")
	bool bAttachWeaponToCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon")
	FTransform WeaponReadyRelativeTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon", meta=(EditCondition="bAttachWeaponToCamera"))
	FTransform CameraWeaponRelativeTransform = FTransform(FRotator(0.0f, -94.0f, 0.0f), FVector(40.0f, 14.0f, -17.0f), FVector(0.72f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon")
	FTransform WeaponLostRelativeTransform = FTransform(FRotator(-42.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, -14.0f), FVector::OneVector);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon", meta=(ClampMin="0.0"))
	float WeaponPoseBlendSpeed = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Shot")
	TObjectPtr<UParticleSystem> ShootEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Shot")
	FName ShootEffectSocketName = TEXT("MuzzleFlash");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Shot")
	TObjectPtr<USoundBase> ShootSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Impact")
	TObjectPtr<USoundBase> BulletImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Impact")
	TObjectPtr<UParticleSystem> BulletImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Impact", meta=(ClampMin="80.0", UIMin="120.0", UIMax="350.0"))
	float BulletRicochetWallClearance = 165.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Impact", meta=(ClampMin="0.0", UIMin="80.0", UIMax="500.0"))
	float BulletRicochetDropDistance = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Impact", meta=(ClampMin="0.0", UIMin="0.0", UIMax="250.0"))
	float BulletRicochetUpLift = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Impact", meta=(ClampMin="100.0", UIMin="300.0", UIMax="1200.0"))
	float BulletRicochetFloorTraceDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Shot")
	TObjectPtr<UAnimationAsset> WeaponShootAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Weapon|Shot")
	bool bUseWeaponMuzzleForBulletFlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<USoundBase> DryFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<USoundBase> KickSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<USoundBase> DodgeSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<USoundBase> DeathSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<USoundBase> HitConfirmSound;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void Shoot();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void Kick();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void Dodge();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void RecoverBullet();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void ConfirmPickupFeedback(const FVector& PickupLocation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|View")
	void ToggleViewMode();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|View")
	void SetThirdPersonView(bool bUseThirdPerson);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Controls")
	void SetMouseSensitivity(float NewSensitivity);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Controls")
	void SetMouseSensitivityNormalized(float NormalizedValue);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Controls")
	void AdjustMouseSensitivity(float Delta);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Controls")
	void ResetMouseSensitivity();

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Controls")
	float GetMouseSensitivity() const { return MouseSensitivity; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Controls")
	float GetMouseSensitivityNormalized() const;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Weapon")
	void SetCameraWeaponRelativeTransform(const FTransform& NewTransform);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Weapon")
	FTransform GetCameraWeaponRelativeTransform() const { return CameraWeaponRelativeTransform; }

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Modes")
	void ToggleImmortalMode();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void ResetForNewRun(const FVector& SpawnLocation, const FRotator& SpawnRotation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void Die();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void DieWithReason(const FText& DeathReason);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerShoot(const FVector& TraceStart, const FVector& TraceEnd, const FVector& ImpactLocation, bool bHitSomething, bool bHitEnemy);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerHitConfirmed(const FVector& ImpactLocation);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerBulletRecovered(const FVector& PickupLocation);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerDryFire();

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerKick(const FVector& KickStart, const FVector& KickEnd, int32 HitEnemyCount);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerDodge(const FVector& DodgeDirection);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerDodgeFailed(const FText& FailReason);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerDeath();

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerViewModeChanged(bool bNowThirdPerson);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerImmortalModeChanged(bool bNowImmortal);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerWeaponStateChanged(bool bBulletReady);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|View")
	bool IsThirdPersonView() const { return bThirdPersonView; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Modes")
	bool IsImmortalMode() const { return bImmortalMode; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Dodge")
	bool IsDodgeReady() const { return bDodgeReady; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Dodge")
	float GetDodgeCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Dodge")
	float GetDodgeCooldownNormalized() const;

protected:
	bool bDead = false;
	bool bKickReady = true;
	bool bDodgeReady = true;
	bool bDodging = false;
	bool bThirdPersonView = false;
	bool bImmortalMode = false;
	bool bWeaponBulletReady = true;
	bool bKickRecovering = false;
	bool bKickFOVPunchActive = false;
	bool bKickFOVReturning = false;

	FVector ActiveDodgeDirection = FVector::ZeroVector;
	float LastMoveForwardInput = 0.0f;
	float LastMoveRightInput = 0.0f;
	float ActiveDodgeElapsed = 0.0f;
	float ActiveDodgePreviousAlpha = 0.0f;
	float RemainingRecoilPitch = 0.0f;
	float DefaultFirstPersonFOV = 90.0f;
	float DefaultThirdPersonFOV = 90.0f;
	float KickFOVElapsed = 0.0f;
	bool bPlayingActionAnimation = false;
	UAnimationAsset* ActiveLocomotionAnimation = nullptr;

	FTimerHandle KickCooldownTimerHandle;
	FTimerHandle DodgeCooldownTimerHandle;
	FTimerHandle FeelStopTimerHandle;
	FTimerHandle ActionAnimationTimerHandle;
	FTimerHandle KickRecoveryTimerHandle;

	TSubclassOf<UAnimInstance> DefaultPlayerAnimClass;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void LookYaw(float Value);
	void LookPitch(float Value);
	void IncreaseMouseSensitivity();
	void DecreaseMouseSensitivity();
	void RestartLevel();
	void ToggleEnemyDetectionRadiusVisualization();
	void ResetKick();
	void FinishKickRecovery();
	void ResetDodge();
	void UpdateDodge(float DeltaSeconds);
	void UpdateRecoil(float DeltaSeconds);
	void UpdateKickFOVPunch(float DeltaSeconds);
	void StartKickFOVPunch();
	void PlayKickFeedback(const FVector& Origin, const FVector& Direction, int32 HitEnemyCount);
	void ApplyFeelStop(float Duration);
	void ResetFeelStop();
	bool TryFindSafeDodgeDirection(FVector& OutDirection) const;
	FVector GetMovementInputDodgeDirection() const;
	bool EvaluateDodgeDirection(const FVector& Direction, float& OutScore) const;
	void ConfigurePlayerMesh();
	void HideFirstPersonHead();
	void ConfigureFullBodyShadowMesh();
	void ConfigureWeapon();
	void ApplyWeaponReadyTransform(bool bSnap);
	void SetWeaponBulletReady(bool bReady, bool bSnap = false);
	void UpdateWeaponPose(float DeltaSeconds);
	USceneComponent* GetWeaponAttachParent() const;
	const FTransform& GetWeaponReadyTargetTransform() const;
	void PlayShootEffect();
	void PlayBulletImpactFeedback(const FHitResult& Hit) const;
	void PlayWeaponShootAnimation();
	FVector GetBulletVisualStartLocation(const FVector& TraceStart) const;
	FVector ResolveBulletDropLocationAfterImpact(const FHitResult& Hit) const;
	void PlayActionAnimation(UAnimationAsset* Animation, float Duration);
	void PlayDeathAnimation();
	void UpdateSimpleLocomotionAnimation();
	void RestoreMovementAnimation();
	void ApplyViewMode();
	void LoadMouseSensitivity();
	void SaveMouseSensitivity() const;
	float GetClampedMouseSensitivity(float Value) const;
	UCameraComponent* GetShootingCamera() const;
	void GetCrosshairTrace(FVector& OutTraceStart, FVector& OutTraceEnd) const;
	AOBBulletPickup* DropBulletAt(const FVector& Location);
};
