#include "OBBulletPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "OBCharacter.h"

AOBBulletPickup::AOBBulletPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(70.0f);
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
}

void AOBBulletPickup::BeginPlay()
{
	Super::BeginPlay();
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AOBBulletPickup::OnPickupOverlap);
}

void AOBBulletPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePickupPresentation(DeltaSeconds, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f);
}

void AOBBulletPickup::SetMeshPresentationOffset(const FVector& RelativeLocation)
{
	if (Mesh)
	{
		Mesh->SetRelativeLocation(RelativeLocation);
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
}

void AOBBulletPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AOBCharacter* Player = Cast<AOBCharacter>(OtherActor))
	{
		Player->RecoverBullet();
		if (PickupSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
		}
		Destroy();
	}
}
