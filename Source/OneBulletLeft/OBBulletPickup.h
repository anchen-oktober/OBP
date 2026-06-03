#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBBulletPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class USoundBase;
class UNiagaraSystem;
class UMaterialInterface;
class USceneComponent;
class AOBCharacter;

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBBulletPickup : public AActor
{
	GENERATED_BODY()

public:
	AOBBulletPickup();

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Presentation")
	void SetMeshPresentationOffset(const FVector& RelativeLocation);

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|Trail")
	void PlayTrailEffect(UObject* WorldContextObject, const FVector& TraceStart, const FVector& TraceEnd) const;

	void PlayTrailEffectToPickup(UObject* WorldContextObject, const FVector& TraceStart, const FVector& TraceEnd, AOBBulletPickup* DestinationPickup) const;
	void BeginIncomingFlight(const FVector& FlightStart);
	void UpdateIncomingFlightLocation(const FVector& FlightLocation);
	void CompleteIncomingFlight();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UPointLightComponent> BeaconLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pickup|Feedback")
	TObjectPtr<UNiagaraSystem> PickupEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	TObjectPtr<UNiagaraSystem> TrailNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	FName TrailStartParameter = TEXT("User.TraceStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	FName TrailEndParameter = TEXT("User.TraceEnd");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="0.01", UIMin="0.05", UIMax="1.0"))
	float TrailFlightDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="1.0", UIMin="300.0", UIMax="5000.0"))
	float TrailTravelSpeed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="0.01", UIMin="0.1", UIMax="3.0"))
	float TrailMaxFlightDuration = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="0.0", UIMin="0.0", UIMax="1.0"))
	float TrailTailDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	TObjectPtr<UMaterialInterface> TravelingBulletMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="0.01", UIMin="0.03", UIMax="0.4"))
	float TravelingBulletScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	FLinearColor TravelingBulletLightColor = FLinearColor(0.16f, 0.82f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="0.0", UIMin="0.0", UIMax="10000.0"))
	float TravelingBulletLightIntensity = 4200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	bool bUseNativePresentationAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation", meta=(DisplayName="Main Mesh Scale", ClampMin="0.01", UIMin="0.05", UIMax="0.5"))
	float MainMeshScale = 0.2625f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float MeshBaseHeight = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float BobHeight = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float BobSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float SpinSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pickup")
	float PickupRadius = 105.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pickup")
	float MagnetRadius = 235.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pickup")
	float MagnetSpeed = 780.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float PulseScale = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float BeaconIntensity = 7200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float BeaconPulseAmount = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation", meta=(ClampMin="0.0", UIMin="200.0", UIMax="1200.0"))
	float BeaconAttenuationRadius = 780.0f;

	UFUNCTION(BlueprintNativeEvent, Category="OneBulletSettings|Presentation")
	void UpdatePickupPresentation(float DeltaSeconds, float RunningTime);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPickupCollected(AOBCharacter* Player);

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Collect(AOBCharacter* Player);

	bool bCollected = false;
	bool bAwaitingIncomingFlight = false;
	TWeakObjectPtr<USceneComponent> FlightDestinationParent;
	FName FlightDestinationSocket = NAME_None;
	FTransform FlightDestinationRelativeTransform = FTransform::Identity;
};
