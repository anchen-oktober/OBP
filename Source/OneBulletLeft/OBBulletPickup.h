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
class UMaterialInstanceDynamic;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> GlowAura;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> VerticalBeam;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio")
	TObjectPtr<USoundBase> PickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio|Landing")
	TObjectPtr<USoundBase> LandingMetalSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio|Landing")
	TObjectPtr<USoundBase> LandingMysticSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio|Landing", meta=(ClampMin="0.0", ClampMax="1.0"))
	float LandingSoundVolume = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Audio|Landing", meta=(ClampMin="0.0", ClampMax="0.5"))
	float LandingMysticEchoDelay = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Pickup|Feedback")
	TObjectPtr<UNiagaraSystem> PickupEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	TObjectPtr<UNiagaraSystem> TrailNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	bool bUseSacredDropTrail = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(EditCondition="bUseSacredDropTrail"))
	TObjectPtr<UNiagaraSystem> SacredDropTrailSystem;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Materials")
	TObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Beam", meta=(DisplayName="Material"))
	TObjectPtr<UMaterialInterface> SacredBeamMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability")
	FLinearColor SacredLightColor = FLinearColor(1.0f, 0.55f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability", meta=(ClampMin="0.1", UIMin="0.5", UIMax="3.0"))
	float GlowAuraScale = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Beam", meta=(DisplayName="Color"))
	FLinearColor VerticalBeamColor = FLinearColor(1.0f, 0.55f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Beam", meta=(DisplayName="Intensity", ClampMin="0.0", UIMin="0.0", UIMax="20.0"))
	float VerticalBeamIntensity = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Beam", meta=(DisplayName="Height", ClampMin="100.0", UIMin="400.0", UIMax="2000.0"))
	float VerticalBeamHeight = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Beam", meta=(DisplayName="Thickness", ClampMin="1.0", UIMin="4.0", UIMax="80.0"))
	float VerticalBeamWidth = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Beam", meta=(DisplayName="Pulse Amount (Brightness + Thickness)", ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="0.5"))
	float VerticalBeamPulseAmount = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Readability|Beam", meta=(DisplayName="Pulse Speed", ClampMin="0.0", UIMin="0.0", UIMax="10.0"))
	float VerticalBeamPulseSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="0.01", UIMin="0.03", UIMax="0.4"))
	float TravelingBulletScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail")
	FLinearColor TravelingBulletLightColor = FLinearColor(1.0f, 0.45f, 0.05f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Trail", meta=(ClampMin="0.0", UIMin="0.0", UIMax="10000.0"))
	float TravelingBulletLightIntensity = 6500.0f;

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
	float BeaconIntensity = 9800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation")
	float BeaconPulseAmount = 3200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|Presentation", meta=(ClampMin="0.0", UIMin="200.0", UIMax="1200.0"))
	float BeaconAttenuationRadius = 1050.0f;

	UFUNCTION(BlueprintNativeEvent, Category="OneBulletSettings|Presentation")
	void UpdatePickupPresentation(float DeltaSeconds, float RunningTime);

	UFUNCTION(BlueprintImplementableEvent, Category="OneBulletSettings|Events")
	void OnPickupCollected(AOBCharacter* Player);

	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Collect(AOBCharacter* Player);
	void SetWorldReadabilityVisible(bool bVisible);
	void ApplyReadabilitySettings();
	void UpdateReadabilityPresentation(float RunningTime);
	void ApplyBeamMaterialColor(float IntensityMultiplier = 1.0f);
	void PlayLandingFeedback();
	void PlayLandingMysticEcho();
	UNiagaraSystem* ResolveDropTrailSystem() const;

	bool bCollected = false;
	bool bAwaitingIncomingFlight = false;
	bool bLandingFeedbackPlayed = false;
	FTimerHandle LandingEchoTimerHandle;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterialInstance;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BeamMaterialInstance;
	TWeakObjectPtr<USceneComponent> FlightDestinationParent;
	FName FlightDestinationSocket = NAME_None;
	FTransform FlightDestinationRelativeTransform = FTransform::Identity;
};
