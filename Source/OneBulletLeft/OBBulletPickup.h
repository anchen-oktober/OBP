#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBBulletPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USoundBase;

UCLASS()
class ONEBULLETLEFT_API AOBBulletPickup : public AActor
{
	GENERATED_BODY()

public:
	AOBBulletPickup();

	UFUNCTION(BlueprintCallable, Category="One Bullet|Presentation")
	void SetMeshPresentationOffset(const FVector& RelativeLocation);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Audio")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Presentation")
	bool bUseNativePresentationAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Presentation")
	float MeshBaseHeight = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Presentation")
	float BobHeight = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Presentation")
	float BobSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="One Bullet|Presentation")
	float SpinSpeed = 180.0f;

	UFUNCTION(BlueprintNativeEvent, Category="One Bullet|Presentation")
	void UpdatePickupPresentation(float DeltaSeconds, float RunningTime);

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
