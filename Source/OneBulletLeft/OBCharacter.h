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
class USoundBase;
class USkeletalMesh;
class AOBBulletPickup;
class AOBEnemy;

UENUM(BlueprintType)
enum class EOBKickResult : uint8
{
	BlockedByCooldown,
	BlockedByDeath,
	Miss,
	Hit
};

UCLASS(Config=Game, PrioritizeCategories = "OneBulletSettings")
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
	TObjectPtr<USkeletalMeshComponent> FullBodyShadowMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Kick|First Person Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> Player_Leg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Visual")
	bool bHideHeadForFirstPerson = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Visual")
	bool bHideBodyForFirstPersonCameraWeapon = true;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="OneBulletSettings|Performance Debug")
	bool bPerformanceDebugEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category="OneBulletSettings|Performance Debug", meta=(ClampMin="0.1", UIMin="0.25", UIMax="5.0"))
	float PerformanceDebugLogInterval = 1.0f;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay")
	float KickRange = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay")
	float KickRadius = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.1", UIMin="1.7", UIMax="2.3"))
	float KickPlayRate = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.18", UIMax="0.35"))
	float KickRecoveryTime = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="20.0", UIMax="60.0"))
	float KickDashDistance = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.01", UIMin="0.05", UIMax="0.14"))
	float KickDashDuration = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay", meta=(ClampMin="0.0", UIMin="500.0", UIMax="1100.0"))
	float KickKnockbackStrength = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="130.0", UIMax="190.0"))
	float KickTraceDistance = 165.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="50.0", UIMax="80.0"))
	float KickTraceRadius = 65.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="3.0"))
	float KickCameraTiltDownAmount = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="1.0", UIMin="12.0", UIMax="28.0"))
	float KickCameraTiltRecoverySpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="4.0"))
	float KickImpactCameraPitchPunch = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="3.0"))
	float KickImpactCameraYawPunch = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float KickImpactHitStopDuration = 0.025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="1.0", ClampMax="180.0", UIMin="60.0", UIMax="90.0"))
	float KickConeAngleDegrees = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay", meta=(ClampMin="1.0", ClampMax="90.0", UIMin="35.0", UIMax="60.0"))
	float KickHalfAngleDegrees = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay", meta=(ClampMin="0.0", UIMin="60.0", UIMax="140.0"))
	float KickEmergencyCloseRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="500.0", UIMax="900.0"))
	float KickPushDistance = 760.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.01", UIMin="0.10", UIMax="0.35"))
	float KickPushDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay", meta=(ClampMin="0.0", UIMin="0.2", UIMax="1.0"))
	float KickStunDuration = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.5", UIMax="0.7"))
	float KickSlowMultiplier = 0.60f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.5", UIMax="1.0"))
	float KickSlowDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.05", UIMax="0.4"))
	float KickRecoveryDuration = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick", meta=(ClampMin="0.0", UIMin="0.0", UIMax="80.0"))
	float KickPlayerLungeDistance = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay", meta=(ClampMin="0.0", UIMin="0.10", UIMax="0.50"))
	float KickCooldown = 0.65f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Kick|Gameplay")
	float NextAllowedKickTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay", meta=(ClampMin="0.0", UIMin="0.03", UIMax="0.15"))
	float SuccessfulKickTouchKillGraceTime = 0.10f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Gameplay", meta=(ClampMin="0.0", UIMin="0.1", UIMax="0.3"))
	float KickAttackCancelWindow = 0.20f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|Debug")
	bool bKickDebugDraw = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel")
	TSubclassOf<UCameraShakeBase> KickCameraShake;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="8.0"))
	float KickFOVPunchAmount = 7.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="20.0"))
	float KickWeaponSwayBack = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="12.0"))
	float KickWeaponSwayDown = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.0", UIMin="0.0", UIMax="15.0"))
	float KickWeaponSwayPitch = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.01", UIMin="0.08", UIMax="0.22"))
	float KickWeaponSwayDuration = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.01", UIMin="0.05", UIMax="0.2"))
	float KickFOVPunchInDuration = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel", meta=(ClampMin="0.01", UIMin="0.05", UIMax="0.3"))
	float KickFOVPunchOutDuration = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel")
	TObjectPtr<UParticleSystem> KickPushEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel")
	TObjectPtr<USoundBase> KickWhooshSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel")
	TObjectPtr<USoundBase> KickImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Feel")
	TObjectPtr<UParticleSystem> KickImpactVFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> KickAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<USkeletalMesh> KickLegMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|First Person Visual")
	FVector KickLegRelativeLocation = FVector(60.0f, 20.0f, -45.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|First Person Visual")
	FRotator KickLegRelativeRotation = FRotator(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|First Person Visual")
	FVector KickLegRelativeScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|First Person Visual")
	bool bOverrideKickLegTransformFromVariables = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|First Person Visual")
	TSubclassOf<AOBCharacter> ExpectedRuntimeCharacterClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|First Person Visual")
	bool bKickUsesRightLeg = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Kick|First Person Visual", meta=(ClampMin="0.0", UIMin="0.03", UIMax="0.08"))
	float KickVisualHideEarlyTime = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|PlayerLeg")
	bool bUseFullBodyMeshAsKickSource = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|PlayerLeg")
	bool bKickVisualCalibrationMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|PlayerLeg", meta=(ClampMin="0.05", UIMin="0.25", UIMax="1.0"))
	float KickVisualCalibrationPlayRate = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|PlayerLeg")
	TArray<FName> KickVisibleBoneNames = { TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|PlayerLeg")
	TArray<FName> KickHiddenBoneNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Kick|Debug")
	bool bKickVisualDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	float KickAnimationDuration = 0.45f;

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

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Kick|PlayerLeg")
	void SetPlayerLegRelativeTransform(const FTransform& NewTransform);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Kick|PlayerLeg")
	void SetPlayerLegRelativeLocation(const FVector& NewLocation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Kick|PlayerLeg")
	void SetPlayerLegRelativeRotation(const FRotator& NewRotation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Kick|PlayerLeg")
	void SetPlayerLegRelativeScale(const FVector& NewScale);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Kick|PlayerLeg")
	FTransform GetPlayerLegRelativeTransform() const;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Modes")
	void ToggleImmortalMode();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Performance Debug")
	void TogglePerformanceDebug();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Performance Debug")
	void SetPerformanceDebugEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Performance Debug")
	void DumpPerformanceSnapshot();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void ResetForNewRun(const FVector& SpawnLocation, const FRotator& SpawnRotation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void Die();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings")
	void DieWithReason(const FText& DeathReason);

	void ShowImmortalModeMsgForThreat(AActor* ThreatSource);
	void HideImmortalModeMsgForThreat(AActor* ThreatSource);

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
	void OnPlayerImmortalModeChanged(bool bNowImmortal);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPlayerWeaponStateChanged(bool bBulletReady);

	UFUNCTION(BlueprintPure, Category="OneBulletSettings")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Modes")
	bool IsImmortalMode() const { return bImmortalMode; }

	UFUNCTION(BlueprintPure, Category="Kick|Gameplay")
	bool DidRecentKickProtectFromEnemy(const AOBEnemy* Enemy) const;

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
	bool bImmortalMode = false;
	bool bWeaponBulletReady = true;
	bool bKickRecovering = false;
	bool bKickFOVPunchActive = false;
	bool bKickFOVReturning = false;
	bool bKickDashing = false;
	bool bKickWeaponSwayActive = false;
	TSet<TWeakObjectPtr<AActor>> ActiveImmortalModeThreatSources;

	FVector ActiveDodgeDirection = FVector::ZeroVector;
	FVector ActiveKickDashDirection = FVector::ZeroVector;
	float LastMoveForwardInput = 0.0f;
	float LastMoveRightInput = 0.0f;
	float ActiveDodgeElapsed = 0.0f;
	float ActiveDodgePreviousAlpha = 0.0f;
	float ActiveKickDashElapsed = 0.0f;
	float ActiveKickDashPreviousAlpha = 0.0f;
	float ActiveKickWeaponSwayElapsed = 0.0f;
	float RemainingRecoilPitch = 0.0f;
	float RemainingKickCameraTiltDown = 0.0f;
	float DefaultFirstPersonFOV = 90.0f;
	float KickFOVElapsed = 0.0f;
	FTransform KickLegInitialRelativeTransform = FTransform::Identity;
	TArray<FName> LastKickMaskHiddenBoneNames;
	FString LastKickMaskHiddenBoneList;
	FString LastKickMaskVisibleWhitelist;
	float LastSuccessfulKickTime = -1000.0f;
	TWeakObjectPtr<AOBEnemy> LastSuccessfulKickEnemy;
	bool bPlayingActionAnimation = false;
	UAnimationAsset* ActiveLocomotionAnimation = nullptr;

	FTimerHandle KickCooldownTimerHandle;
	FTimerHandle DodgeCooldownTimerHandle;
	FTimerHandle FeelStopTimerHandle;
	FTimerHandle ActionAnimationTimerHandle;
	FTimerHandle KickRecoveryTimerHandle;
	FTimerHandle KickLegAnimationTimerHandle;
	FTimerHandle PerformanceDebugTimerHandle;

	TSubclassOf<UAnimInstance> DefaultPlayerAnimClass;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void LookYaw(float Value);
	void LookPitch(float Value);
	void IncreaseMouseSensitivity();
	void DecreaseMouseSensitivity();
	void ToggleMouseSensitivityUI();
	void RestartLevel();
	void ResetKick();
	EOBKickResult GameplayKick();
	void FinishKickRecovery();
	void ResetDodge();
	void UpdateDodge(float DeltaSeconds);
	void UpdateKickDash(float DeltaSeconds);
	void UpdateRecoil(float DeltaSeconds);
	void UpdateKickCameraTilt(float DeltaSeconds);
	void UpdateKickFOVPunch(float DeltaSeconds);
	void StartKickFOVPunch();
	void StartKickDash(const FVector& Direction);
	void StartKickWeaponSway();
	void PlayKickStartFeedback();
	void PlayKickImpactFeedback(const FVector& Origin, const FVector& Direction, int32 HitEnemyCount);
	bool IsEnemyInKickZone(const AOBEnemy* Enemy, const FVector& Start, const FVector& Forward, float MinForwardDot, float& OutDistance, float& OutDot) const;
	TArray<AOBEnemy*> GatherKickTargets(FVector& OutStart, FVector& OutEnd, FVector& OutForward, FVector& OutOverlapCenter, float& OutOverlapRadius, int32& OutCandidateCount, FString& OutTargetDebugText) const;
	void ApplyFeelStop(float Duration);
	void ResetFeelStop();
	bool TryFindSafeDodgeDirection(FVector& OutDirection) const;
	FVector GetMovementInputDodgeDirection() const;
	bool EvaluateDodgeDirection(const FVector& Direction, float& OutScore) const;
	void ConfigurePlayerMesh();
	void ConfigureKickLeg();
	void ConfigureFirstPersonBodyVisibility();
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
	bool ApplyKickLegOnlyMask();
	void ResetKickBoneMask();
	void ShowKickVisualDebugMessage(const FString& Message) const;
	bool PlayKickLegAnimation();
	void FinishKickLegAnimation();
	FVector GetBulletVisualStartLocation(const FVector& TraceStart) const;
	FVector ResolveBulletDropLocationAfterImpact(const FHitResult& Hit) const;
	void PlayActionAnimation(UAnimationAsset* Animation, float Duration, float PlayRate = 1.0f, bool bUseFullAnimationLength = false);
	void PlayDeathAnimation();
	void UpdateSimpleLocomotionAnimation();
	void RestoreMovementAnimation();
	void LoadMouseSensitivity();
	void SaveMouseSensitivity() const;
	float GetClampedMouseSensitivity(float Value) const;
	void GetCrosshairTrace(FVector& OutTraceStart, FVector& OutTraceEnd) const;
	AOBBulletPickup* DropBulletAt(const FVector& Location);
	void UpdatePerformanceDebug();
	FString BuildPerformanceSnapshotText() const;
};
