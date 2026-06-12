#include "OBBulletPickup.h"

#include "Components/PointLightComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "OBBulletTrailActor.h"
#include "OBCharacter.h"
#include "OBGameState.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogOBBulletReadability, Log, All);

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
	Mesh->SetRelativeScale3D(FVector(MainMeshScale));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	GlowAura = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GlowAura"));
	GlowAura->SetupAttachment(RootComponent);
	GlowAura->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GlowAura->SetCastShadow(false);
	GlowAura->TranslucencySortPriority = 80;
	if (SphereMesh.Succeeded())
	{
		GlowAura->SetStaticMesh(SphereMesh.Object);
	}

	VerticalBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VerticalBeam"));
	VerticalBeam->SetupAttachment(RootComponent);
	VerticalBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VerticalBeam->SetCastShadow(false);
	VerticalBeam->TranslucencySortPriority = 70;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VerticalBeam->SetStaticMesh(CylinderMesh.Object);
	}

	BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
	BeaconLight->SetupAttachment(RootComponent);
	BeaconLight->SetRelativeLocation(FVector(0.0f, 0.0f, 18.0f));
	BeaconLight->SetLightColor(SacredLightColor);
	BeaconLight->SetIntensity(BeaconIntensity);
	BeaconLight->SetAttenuationRadius(BeaconAttenuationRadius);
	BeaconLight->SetCastShadows(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultGlowMaterial(
		TEXT("/Game/MilitaryWeapDark/FX/Materials/M_GlowSphere_01.M_GlowSphere_01"));
	if (DefaultGlowMaterial.Succeeded())
	{
		GlowMaterial = DefaultGlowMaterial.Object;
		TravelingBulletMaterial = DefaultGlowMaterial.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultSacredBeamMaterial(
		TEXT("/Game/LevelPrototyping/Interactable/JumpPad/Assets/Materials/M_SimpleGlow.M_SimpleGlow"));
	if (DefaultSacredBeamMaterial.Succeeded())
	{
		SacredBeamMaterial = DefaultSacredBeamMaterial.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultHolyTrail(
		TEXT("/Game/Assets/VFX/ArrowTrail/FX/NS_ArrowTrail_Holy.NS_ArrowTrail_Holy"));
	if (DefaultHolyTrail.Succeeded())
	{
		SacredDropTrailSystem = DefaultHolyTrail.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultLandingMetalSound(
		TEXT("/Game/Sound/cue/Shield_Metal_Impact_1-3_Cue.Shield_Metal_Impact_1-3_Cue"));
	if (DefaultLandingMetalSound.Succeeded())
	{
		LandingMetalSound = DefaultLandingMetalSound.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultLandingMysticSound(
		TEXT("/Game/Sound/cue/Magical_Interface_5-1_Cue.Magical_Interface_5-1_Cue"));
	if (DefaultLandingMysticSound.Succeeded())
	{
		LandingMysticSound = DefaultLandingMysticSound.Object;
	}
}

void AOBBulletPickup::BeginPlay()
{
	Super::BeginPlay();
	PickupSphere->SetSphereRadius(PickupRadius);
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AOBBulletPickup::OnPickupOverlap);
	ApplyReadabilitySettings();
	UE_LOG(
		LogOBBulletReadability,
		Log,
		TEXT("%s readability initialized: Glow=%s, Beam=%s, Trail=%s, BeamPulse=%.3f @ %.3f"),
		*GetName(),
		*GetNameSafe(GlowMaterial),
		*GetNameSafe(SacredBeamMaterial),
		*GetNameSafe(ResolveDropTrailSystem()),
		VerticalBeamPulseAmount,
		VerticalBeamPulseSpeed);
}

void AOBBulletPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bAwaitingIncomingFlight)
	{
		return;
	}

	const float RunningTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	UpdatePickupPresentation(DeltaSeconds, RunningTime);
	UpdateReadabilityPresentation(RunningTime);

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
		UNiagaraSystem* ActiveTrailSystem = ResolveDropTrailSystem();
		const float DistanceDuration = FVector::Distance(TraceStart, TraceEnd) / FMath::Max(TrailTravelSpeed, 1.0f);
		const float ActualFlightDuration = FMath::Clamp(
			DistanceDuration,
			FMath::Max(TrailFlightDuration, 0.01f),
			FMath::Max(TrailMaxFlightDuration, TrailFlightDuration));
		TrailActor->InitializeTrail(
			ActiveTrailSystem,
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

void AOBBulletPickup::BeginIncomingFlight(const FVector& FlightStart)
{
	if (bCollected)
	{
		return;
	}

	bAwaitingIncomingFlight = true;
	bLandingFeedbackPlayed = false;
	GetWorldTimerManager().ClearTimer(LandingEchoTimerHandle);
	if (RootComponent && RootComponent->GetAttachParent())
	{
		FlightDestinationParent = RootComponent->GetAttachParent();
		FlightDestinationSocket = RootComponent->GetAttachSocketName();
		FlightDestinationRelativeTransform = RootComponent->GetRelativeTransform();
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	SetActorLocation(FlightStart, false);
	SetActorHiddenInGame(false);
	SetWorldReadabilityVisible(false);
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(FVector(MainMeshScale));
	}
	if (PickupSphere)
	{
		PickupSphere->SetGenerateOverlapEvents(false);
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AOBBulletPickup::UpdateIncomingFlightLocation(const FVector& FlightLocation)
{
	if (bAwaitingIncomingFlight && !bCollected)
	{
		SetActorLocation(FlightLocation, false);
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
	if (FlightDestinationParent.IsValid())
	{
		AttachToComponent(FlightDestinationParent.Get(), FAttachmentTransformRules::KeepRelativeTransform, FlightDestinationSocket);
		RootComponent->SetRelativeTransform(FlightDestinationRelativeTransform);
		FlightDestinationParent.Reset();
	}
	if (PickupSphere)
	{
		PickupSphere->SetCollisionProfileName(TEXT("Trigger"));
		PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PickupSphere->SetGenerateOverlapEvents(true);
	}
	SetWorldReadabilityVisible(true);
	PlayLandingFeedback();
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
	Mesh->SetRelativeScale3D(FVector(MainMeshScale * Pulse));
}

void AOBBulletPickup::UpdateReadabilityPresentation(float RunningTime)
{
	const float Bob = FMath::Sin(RunningTime * BobSpeed) * BobHeight;
	if (BeaconLight)
	{
		const float PulseAlpha = 0.5f + FMath::Sin(RunningTime * BobSpeed) * 0.5f;
		BeaconLight->SetLightColor(SacredLightColor);
		BeaconLight->SetIntensity(FMath::Max(BeaconIntensity, 0.0f) + PulseAlpha * FMath::Max(BeaconPulseAmount, 0.0f));
		BeaconLight->SetAttenuationRadius(FMath::Max(BeaconAttenuationRadius, 0.0f));
	}
	if (GlowAura)
	{
		const float AuraPulse = 1.0f + PulseScale * 0.35f * FMath::Sin(RunningTime * BobSpeed);
		GlowAura->SetRelativeLocation(FVector(0.0f, 0.0f, MeshBaseHeight + Bob));
		GlowAura->SetRelativeScale3D(FVector(MainMeshScale * GlowAuraScale * AuraPulse));
	}
	if (VerticalBeam && !bAwaitingIncomingFlight)
	{
		const float BeamPulse = FMath::Max(
			1.0f + FMath::Sin(RunningTime * VerticalBeamPulseSpeed) * VerticalBeamPulseAmount,
			0.05f);
		VerticalBeam->SetRelativeScale3D(FVector(
			VerticalBeamWidth / 100.0f * BeamPulse,
			VerticalBeamWidth / 100.0f * BeamPulse,
			VerticalBeamHeight / 100.0f));
		ApplyBeamMaterialColor(BeamPulse);
	}
}

void AOBBulletPickup::SetWorldReadabilityVisible(bool bVisible)
{
	if (VerticalBeam)
	{
		VerticalBeam->SetVisibility(bVisible, true);
	}
}

void AOBBulletPickup::ApplyReadabilitySettings()
{
	if (GlowAura)
	{
		GlowAura->SetRelativeLocation(FVector(0.0f, 0.0f, MeshBaseHeight));
		GlowAura->SetRelativeScale3D(FVector(MainMeshScale * GlowAuraScale));
		if (GlowMaterial)
		{
			GlowMaterialInstance = GlowAura->CreateDynamicMaterialInstance(0, GlowMaterial);
		}
	}
	if (VerticalBeam)
	{
		VerticalBeam->SetRelativeLocation(FVector(0.0f, 0.0f, VerticalBeamHeight * 0.5f));
		VerticalBeam->SetRelativeScale3D(FVector(
			VerticalBeamWidth / 100.0f,
			VerticalBeamWidth / 100.0f,
			VerticalBeamHeight / 100.0f));
		if (SacredBeamMaterial)
		{
			BeamMaterialInstance = VerticalBeam->CreateDynamicMaterialInstance(0, SacredBeamMaterial);
		}
	}

	const TArray<UMaterialInstanceDynamic*> ReadabilityMaterials =
	{
		GlowMaterialInstance
	};
	for (UMaterialInstanceDynamic* Material : ReadabilityMaterials)
	{
		if (!Material)
		{
			continue;
		}
		Material->SetVectorParameterValue(TEXT("Color"), SacredLightColor);
		Material->SetVectorParameterValue(TEXT("Tint"), SacredLightColor);
		Material->SetVectorParameterValue(TEXT("EmissiveColor"), SacredLightColor);
		Material->SetVectorParameterValue(TEXT("Emissive Color"), SacredLightColor);
	}
	ApplyBeamMaterialColor();

	if (BeaconLight)
	{
		BeaconLight->SetLightColor(SacredLightColor);
		BeaconLight->SetIntensity(FMath::Max(BeaconIntensity, 0.0f));
		BeaconLight->SetAttenuationRadius(FMath::Max(BeaconAttenuationRadius, 0.0f));
	}
}

void AOBBulletPickup::ApplyBeamMaterialColor(float IntensityMultiplier)
{
	if (!BeamMaterialInstance)
	{
		return;
	}

	const float EffectiveIntensity = FMath::Max(VerticalBeamIntensity, 0.0f) * FMath::Max(IntensityMultiplier, 0.0f);
	FLinearColor EffectiveColor = VerticalBeamColor;
	EffectiveColor.R *= EffectiveIntensity;
	EffectiveColor.G *= EffectiveIntensity;
	EffectiveColor.B *= EffectiveIntensity;

	BeamMaterialInstance->SetVectorParameterValue(TEXT("Color"), EffectiveColor);
	BeamMaterialInstance->SetVectorParameterValue(TEXT("Tint"), EffectiveColor);
	BeamMaterialInstance->SetVectorParameterValue(TEXT("EmissiveColor"), EffectiveColor);
	BeamMaterialInstance->SetVectorParameterValue(TEXT("Emissive Color"), EffectiveColor);
}

void AOBBulletPickup::PlayLandingFeedback()
{
	if (bLandingFeedbackPlayed || !GetWorld())
	{
		return;
	}

	bLandingFeedbackPlayed = true;
	UE_LOG(
		LogOBBulletReadability,
		Log,
		TEXT("%s landed: Metal=%s, Mystic=%s, EchoDelay=%.2fs"),
		*GetName(),
		*GetNameSafe(LandingMetalSound),
		*GetNameSafe(LandingMysticSound),
		LandingMysticEchoDelay);
	if (LandingMetalSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			LandingMetalSound,
			GetActorLocation(),
			FMath::Clamp(LandingSoundVolume, 0.0f, 1.0f),
			FMath::FRandRange(0.96f, 1.04f));
	}

	if (LandingMysticSound)
	{
		const float EchoDelay = FMath::Max(LandingMysticEchoDelay, 0.0f);
		if (EchoDelay <= KINDA_SMALL_NUMBER)
		{
			PlayLandingMysticEcho();
		}
		else
		{
			GetWorldTimerManager().SetTimer(
				LandingEchoTimerHandle,
				this,
				&AOBBulletPickup::PlayLandingMysticEcho,
				EchoDelay,
				false);
		}
	}
}

void AOBBulletPickup::PlayLandingMysticEcho()
{
	if (LandingMysticSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			LandingMysticSound,
			GetActorLocation(),
			FMath::Clamp(LandingSoundVolume * 0.45f, 0.0f, 1.0f),
			0.92f);
	}
}

UNiagaraSystem* AOBBulletPickup::ResolveDropTrailSystem() const
{
	return bUseSacredDropTrail && SacredDropTrailSystem
		? SacredDropTrailSystem.Get()
		: TrailNiagaraSystem.Get();
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
	if (PickupEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, PickupEffect, GetActorLocation(), GetActorRotation());
	}
	Destroy();
}
