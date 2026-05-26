#include "OBBulletPickup.h"

#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "OBBulletTrailActor.h"
#include "OBCharacter.h"
#include "OBGameState.h"

AOBBulletPickup::AOBBulletPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(PickupRadius);
	PickupSphere->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = PickupSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
	BeaconLight->SetupAttachment(RootComponent);
	BeaconLight->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	BeaconLight->SetLightColor(FLinearColor(0.14f, 0.78f, 1.0f));
	BeaconLight->SetIntensity(BeaconIntensity);
	BeaconLight->SetAttenuationRadius(420.0f);
}

void AOBBulletPickup::BeginPlay()
{
	Super::BeginPlay();
	PickupSphere->SetSphereRadius(PickupRadius);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AOBBulletPickup::OnPickupOverlap);
}

void AOBBulletPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAwaitingIncomingFlight)
	{
		return;
	}

	UpdatePickupPresentation(DeltaSeconds, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);

	const AOBGameState* State = GetWorld() ? GetWorld()->GetGameState<AOBGameState>() : nullptr;
	AOBCharacter* Player = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (State && !State->HasBullet() && Player)
	{
		const float Distance = FVector::Dist(GetActorLocation(), Player->GetActorLocation());
		const float PlayerRadius = Player->GetCapsuleComponent() ? Player->GetCapsuleComponent()->GetScaledCapsuleRadius() : 0.0f;
		if (Distance <= PickupRadius + PlayerRadius)
		{
			Collect(Player);
			return;
		}

		if (Distance < MagnetRadius)
		{
			const FVector Target = Player->GetActorLocation() + FVector(0.0f, 0.0f, 35.0f);
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(), Target, DeltaSeconds, MagnetSpeed), false);
		}
	}
}

void AOBBulletPickup::SetMeshPresentationOffset(const FVector& RelativeLocation)
{
	if (Mesh)
	{
		Mesh->SetRelativeLocation(RelativeLocation);
	}
}

void AOBBulletPickup::PlayTrailEffect(UObject* WorldContextObject, const FVector& TraceStart, const FVector& TraceEnd) const
{
	PlayTrailEffectToPickup(WorldContextObject, TraceStart, TraceEnd, nullptr);
}

void AOBBulletPickup::PlayTrailEffectToPickup(UObject* WorldContextObject, const FVector& TraceStart, const FVector& TraceEnd, AOBBulletPickup* DestinationPickup) const
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AOBBulletTrailActor* TrailActor = World->SpawnActor<AOBBulletTrailActor>(
		AOBBulletTrailActor::StaticClass(), TraceStart, (TraceEnd - TraceStart).Rotation(), Params);
	if (TrailActor)
	{
		const float DistanceDuration = FVector::Distance(TraceStart, TraceEnd) / FMath::Max(TrailTravelSpeed, 1.0f);
		const float ActualFlightDuration = FMath::Clamp(
			DistanceDuration,
			FMath::Max(TrailFlightDuration, 0.01f),
			FMath::Max(TrailMaxFlightDuration, TrailFlightDuration));
		TrailActor->InitializeTrail(
			TrailNiagaraSystem,
			TraceStart,
			TraceEnd,
			TrailStartParameter,
			TrailEndParameter,
			ActualFlightDuration,
			TrailTailDuration,
			DestinationPickup,
			TravelingBulletMaterial,
			TravelingBulletScale,
			TravelingBulletLightColor,
			TravelingBulletLightIntensity);
	}
}

void AOBBulletPickup::BeginIncomingFlight()
{
	if (bCollected)
	{
		return;
	}

	bAwaitingIncomingFlight = true;
	SetActorHiddenInGame(true);
	if (PickupSphere)
	{
		PickupSphere->SetGenerateOverlapEvents(false);
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AOBBulletPickup::CompleteIncomingFlight()
{
	if (bCollected)
	{
		return;
	}

	bAwaitingIncomingFlight = false;
	SetActorHiddenInGame(false);
	if (PickupSphere)
	{
		PickupSphere->SetCollisionProfileName(TEXT("Trigger"));
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PickupSphere->SetGenerateOverlapEvents(true);
	}
}

void AOBBulletPickup::UpdatePickupPresentation_Implementation(float DeltaSeconds, float RunningTime)
{
	if (!bUseNativePresentationAnimation)
	{
		return;
	}

	AddActorWorldRotation(FRotator(0.0f, SpinSpeed * DeltaSeconds, 0.0f));
	const float Bob = FMath::Sin(RunningTime * BobSpeed) * BobHeight;
	SetMeshPresentationOffset(FVector(0.0f, 0.0f, MeshBaseHeight + Bob));
	const float Pulse = 1.0f + FMath::Sin(RunningTime * BobSpeed) * PulseScale;
	Mesh->SetRelativeScale3D(FVector(0.35f * Pulse));
	if (BeaconLight)
	{
		BeaconLight->SetIntensity(BeaconIntensity + FMath::Max(0.0f, FMath::Sin(RunningTime * BobSpeed)) * BeaconPulseAmount);
	}
}

void AOBBulletPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AOBCharacter* Player = Cast<AOBCharacter>(OtherActor))
	{
		Collect(Player);
	}
}

void AOBBulletPickup::Collect(AOBCharacter* Player)
{
	if (bCollected || bAwaitingIncomingFlight || !Player)
	{
		return;
	}

	bCollected = true;
	Player->RecoverBullet();
	Player->ConfirmPickupFeedback(GetActorLocation());
	OnPickupCollected(Player);
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
	Destroy();
}
