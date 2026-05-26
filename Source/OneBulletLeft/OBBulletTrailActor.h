#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBBulletTrailActor.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInterface;
class AOBBulletPickup;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class ONEBULLETLEFT_API AOBBulletTrailActor : public AActor
{
	GENERATED_BODY()

public:
	AOBBulletTrailActor();

	void InitializeTrail(
		UNiagaraSystem* TrailSystem,
		const FVector& TraceStart,
		const FVector& TraceEnd,
		FName StartParameter,
		FName EndParameter,
		float FlightDuration,
		float TailDuration,
		AOBBulletPickup* DestinationPickup,
		UMaterialInterface* BulletMaterial,
		float BulletScale,
		const FLinearColor& BulletLightColor,
		float BulletLightIntensity);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TrailComponent;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TravelingBulletMesh;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> TravelingBulletLight;

	UPROPERTY()
	TObjectPtr<AOBBulletPickup> DestinationPickup;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float Duration = 0.12f;
	float Elapsed = 0.0f;
	float TailLife = 0.22f;
	bool bArrived = false;
};
