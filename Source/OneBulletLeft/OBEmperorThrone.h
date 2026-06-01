#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBEmperorThrone.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UAnimationAsset;
class USkeletalMesh;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EOBEmperorAnimationState : uint8
{
	Idle,
	Clapping
};

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBEmperorThrone : public AActor
{
	GENERATED_BODY()

public:
	AOBEmperorThrone();

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ThroneBase;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ThroneBack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> ThroneSeat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> LeftArmRest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> RightArmRest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> BossMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Boss")
	TObjectPtr<USkeletalMesh> BossModel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Boss")
	FTransform BossRelativeTransform = FTransform(FRotator(0.0f, -90.0f, 0.0f), FVector(32.0f, 0.0f, 92.0f), FVector(1.0f));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	EOBEmperorAnimationState AnimationState = EOBEmperorAnimationState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	bool bAutoClapOnPlayerDeath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> SittingAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Animation")
	TObjectPtr<UAnimationAsset> SittingClapAnimation;

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Animation")
	void PlayIdle();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Animation")
	void PlayClap();

	UFUNCTION(BlueprintPure, Category="OneBulletSettings|Animation")
	bool IsClapping() const { return AnimationState == EOBEmperorAnimationState::Clapping; }

protected:
	bool bWasGameOver = false;

	void ConfigureMeshes();
	void ConfigureBossMesh();
	void ConfigureMaterials();
	void PlayCurrentAnimation();
	void SetMeshColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color);
	UStaticMeshComponent* MakeMeshComponent(const FName Name, USceneComponent* Parent);
};
