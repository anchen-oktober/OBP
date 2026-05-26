#include "OBBulletTrailActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "OBBulletPickup.h"

AOBBulletTrailActor::AOBBulletTrailActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
	TrailComponent->SetupAttachment(RootComponent);
	TrailComponent->SetAutoActivate(false);

	TravelingBulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TravelingBulletMesh"));
	TravelingBulletMesh->SetupAttachment(RootComponent);
	TravelingBulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TravelingBulletMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		TravelingBulletMesh->SetStaticMesh(SphereMesh.Object);
	}

	TravelingBulletLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("TravelingBulletLight"));
	TravelingBulletLight->SetupAttachment(RootComponent);
	TravelingBulletLight->SetAttenuationRadius(220.0f);
}

void AOBBulletTrailActor::InitializeTrail(
	UNiagaraSystem* TrailSystem,
	const FVector& TraceStart,
	const FVector& TraceEnd,
	FName StartParameter,
	FName EndParameter,
	float FlightDuration,
	float TailDuration,
	AOBBulletPickup* InDestinationPickup,
	UMaterialInterface* BulletMaterial,
	float BulletScale,
	const FLinearColor& BulletLightColor,
	float BulletLightIntensity)
{
	StartLocation = TraceStart;
	EndLocation = TraceEnd;
	Duration = FMath::Max(FlightDuration, 0.01f);
	TailLife = FMath::Max(TailDuration, 0.0f);
	DestinationPickup = InDestinationPickup;
	SetActorLocationAndRotation(StartLocation, (EndLocation - StartLocation).Rotation());

	if (DestinationPickup)
	{
		DestinationPickup->BeginIncomingFlight(StartLocation);
	}

	if (TrailSystem && TrailComponent)
	{
		TrailComponent->SetAsset(TrailSystem);
		TrailComponent->SetVariableVec3(StartParameter, TraceStart);
		TrailComponent->SetVariableVec3(EndParameter, TraceEnd);
		TrailComponent->Activate(true);
	}

	if (TravelingBulletMesh)
	{
		TravelingBulletMesh->SetVisibility(!DestinationPickup);
		TravelingBulletMesh->SetRelativeScale3D(FVector(FMath::Max(BulletScale, 0.01f)));
		if (BulletMaterial)
		{
			TravelingBulletMesh->SetMaterial(0, BulletMaterial);
		}
	}
	if (TravelingBulletLight)
	{
		TravelingBulletLight->SetVisibility(!DestinationPickup);
		TravelingBulletLight->SetLightColor(BulletLightColor);
		TravelingBulletLight->SetIntensity(FMath::Max(BulletLightIntensity, 0.0f));
	}
}

void AOBBulletTrailActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bArrived)
	{
		return;
	}

	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(StartLocation, EndLocation, Alpha));
	if (DestinationPickup)
	{
		DestinationPickup->UpdateIncomingFlightLocation(GetActorLocation());
	}

	if (Alpha >= 1.0f)
	{
		bArrived = true;
		if (TrailComponent)
		{
			TrailComponent->Deactivate();
		}
		if (TravelingBulletMesh)
		{
			TravelingBulletMesh->SetVisibility(false);
		}
		if (TravelingBulletLight)
		{
			TravelingBulletLight->SetVisibility(false);
		}
		if (DestinationPickup)
		{
			DestinationPickup->CompleteIncomingFlight();
		}
		SetLifeSpan(FMath::Max(TailLife, 0.01f));
	}
}
