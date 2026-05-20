#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "OBCharacter.generated.h"

class UCameraComponent;
class USoundBase;

UCLASS()
class ONEBULLETLEFT_API AOBCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOBCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> FullBodyShadowMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Visual")
	bool bHideHeadForFirstPerson = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Shooting")
	float ShootRange = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Kick")
	float KickRange = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Kick")
	float KickRadius = 125.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Kick")
	float KickCooldown = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Dodge")
	float DodgeDistance = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Dodge")
	float DodgeDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Dodge")
	float DodgeCooldown = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Dodge")
	float DodgeEnemyClearance = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Dodge")
	bool bPreferMovementDirectionDodge = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Dodge")
	FKey DodgeKey = EKeys::LeftShift;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Audio")
	TObjectPtr<USoundBase> ShootSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Audio")
	TObjectPtr<USoundBase> DryFireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Audio")
	TObjectPtr<USoundBase> KickSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Audio")
	TObjectPtr<USoundBase> DodgeSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Audio")
	TObjectPtr<USoundBase> DeathSound;

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void Shoot();

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void Kick();

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void Dodge();

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void RecoverBullet();

	UFUNCTION(BlueprintCallable, Category="One Bullet")
	void Die();

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|Events")
	void OnPlayerShoot(const FVector& TraceStart, const FVector& TraceEnd, const FVector& ImpactLocation, bool bHitSomething, bool bHitEnemy);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|Events")
	void OnPlayerDryFire();

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|Events")
	void OnPlayerKick(const FVector& KickStart, const FVector& KickEnd, int32 HitEnemyCount);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|Events")
	void OnPlayerDodge(const FVector& DodgeDirection);

	UFUNCTION(BlueprintImplementableEvent, Category="One Bullet|Events")
	void OnPlayerDeath();

	UFUNCTION(BlueprintPure, Category="One Bullet")
	bool IsDead() const { return bDead; }

protected:
	bool bDead = false;
	bool bKickReady = true;
	bool bDodgeReady = true;
	bool bDodging = false;

	FVector ActiveDodgeDirection = FVector::ZeroVector;
	float ActiveDodgeElapsed = 0.0f;
	float ActiveDodgePreviousAlpha = 0.0f;

	FTimerHandle KickCooldownTimerHandle;
	FTimerHandle DodgeCooldownTimerHandle;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void LookYaw(float Value);
	void LookPitch(float Value);
	void RestartLevel();
	void ResetKick();
	void ResetDodge();
	void UpdateDodge(float DeltaSeconds);
	void PollDodgeInput();
	bool TryFindSafeDodgeDirection(FVector& OutDirection) const;
	bool EvaluateDodgeDirection(const FVector& Direction, float& OutScore) const;
	void ConfigurePlayerMesh();
	void HideFirstPersonHead();
	void ConfigureFullBodyShadowMesh();
	void DropBulletAt(const FVector& Location);
};
