#include "OBPanicAudioManager.h"

#include "AIController.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "OBEnemy.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogOBPanicAudio, Log, All);

AOBPanicAudioManager::AOBPanicAudioManager()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	HeartbeatComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Heartbeat"));
	HeartbeatComponent->SetupAttachment(Root);
	HeartbeatComponent->bAutoActivate = false;
	HeartbeatComponent->bAllowSpatialization = false;

	LowDroneComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("LowDrone"));
	LowDroneComponent->SetupAttachment(Root);
	LowDroneComponent->bAutoActivate = false;
	LowDroneComponent->bAllowSpatialization = false;
}

void AOBPanicAudioManager::BeginPlay()
{
	Super::BeginPlay();
	StartAmbientHorrorLoop();
}

void AOBPanicAudioManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	KeepLoopLayersAlive();
}

void AOBPanicAudioManager::HandlePlayerShot(AActor* AudioFocus)
{
	PlayShotSound(AudioFocus);
	StartPanicAudio(AudioFocus);
}

void AOBPanicAudioManager::HandleBulletPickedUp(AActor* AudioFocus)
{
	StopPanicAudio(AudioFocus, true);
	ApplyReliefReactionToEnemies(AudioFocus);
}

void AOBPanicAudioManager::PlayShotSound(AActor* AudioFocus)
{
	if (AudioFocus)
	{
		SetActorLocation(AudioFocus->GetActorLocation());
	}

	if (!ShotSound)
	{
		DebugMissingSound(TEXT("ShotSound"));
		return;
	}

	UGameplayStatics::PlaySound2D(this, ShotSound, FMath::Max(ShotVolume, 0.0f), ShotPitch);
}

void AOBPanicAudioManager::StartPanicAudio(AActor* AudioFocus)
{
	if (bPanicAudioActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(PanicAudioDelayTimerHandle))
	{
		return;
	}

	PendingAudioFocus = AudioFocus;
	const float Delay = FMath::FRandRange(PanicStartDelayMin, PanicStartDelayMax);
	World->GetTimerManager().SetTimer(PanicAudioDelayTimerHandle, this, &AOBPanicAudioManager::StartPanicAudioAfterDelay, Delay, false);
}

void AOBPanicAudioManager::StopPanicAudio(AActor* AudioFocus, bool bPlayBulletPickupSound)
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(PanicAudioDelayTimerHandle);
		World->GetTimerManager().ClearTimer(RoarStartTimerHandle);
		World->GetTimerManager().ClearTimer(FootstepsStartTimerHandle);
	}

	if (AudioFocus)
	{
		SetActorLocation(AudioFocus->GetActorLocation());
	}

	if (bPlayBulletPickupSound)
	{
		if (BulletPickupSound)
		{
			UGameplayStatics::PlaySound2D(this, BulletPickupSound, FMath::Max(BulletPickupVolume, 0.0f), 1.0f);
		}
		else
		{
			DebugMissingSound(TEXT("BulletPickupSound"));
		}
	}

	StopHeartbeatLayer();
	StopCrowdRoarLayer();
	StopCrowdFootstepsLayer();
	StopLowDroneLayer();

	PendingAudioFocus.Reset();
	bPanicAudioActive = false;
}

void AOBPanicAudioManager::StartAmbientHorrorLoop()
{
	ScheduleNextAmbientHorror();
}

void AOBPanicAudioManager::PlayRandomAmbientHorror()
{
	const float Roll = FMath::FRand();
	if (Roll > AmbientPlayChance)
	{
		ScheduleNextAmbientHorror();
		return;
	}

	if (FMath::RandBool())
	{
		PlayAmbientSound(WhisperAmbientSound, WhisperVolume, TEXT("WhisperAmbientSound"));
	}
	else
	{
		PlayAmbientSound(GhostAmbientSound, GhostVolume, TEXT("GhostAmbientSound"));
	}

	ScheduleNextAmbientHorror();
}

void AOBPanicAudioManager::ApplyReliefReactionToEnemies(AActor* PlayerActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* Player = PlayerActor ? PlayerActor : UGameplayStatics::GetPlayerPawn(this, 0);
	for (TActorIterator<AOBEnemy> It(World); It; ++It)
	{
		AOBEnemy* Enemy = *It;
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}

		const float SlowDuration = FMath::FRandRange(ReliefSlowDurationMin, ReliefSlowDurationMax);
		const float SpeedMultiplier = FMath::FRandRange(ReliefSpeedMultiplierMin, ReliefSpeedMultiplierMax);
		const float StepBackDistance = FMath::FRandRange(ReliefStepBackDistanceMin, ReliefStepBackDistanceMax);
		const float StepBackDuration = FMath::FRandRange(ReliefStepBackDurationMin, ReliefStepBackDurationMax);
		Enemy->ApplyBulletPickupReliefReaction(Player, SlowDuration, SpeedMultiplier, StepBackDistance, StepBackDuration);
	}
}

void AOBPanicAudioManager::StartPanicAudioAfterDelay()
{
	if (bPanicAudioActive)
	{
		return;
	}

	if (AActor* AudioFocus = PendingAudioFocus.Get())
	{
		SetActorLocation(AudioFocus->GetActorLocation());
	}

	bPanicAudioActive = true;
	StartHeartbeatLayer();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RoarStartTimerHandle, this, &AOBPanicAudioManager::StartCrowdRoarLayer, RoarDelayAfterHeartbeat, false);
		World->GetTimerManager().SetTimer(FootstepsStartTimerHandle, this, &AOBPanicAudioManager::StartCrowdFootstepsLayer, RoarDelayAfterHeartbeat + FootstepsDelayAfterRoar, false);
	}
}

void AOBPanicAudioManager::StartHeartbeatLayer()
{
	bHeartbeatLayerActive = true;
	if (!HeartbeatComponent || !HeartbeatSound)
	{
		DebugMissingSound(TEXT("HeartbeatSound"));
		return;
	}

	HeartbeatComponent->SetSound(HeartbeatSound);
	HeartbeatComponent->SetVolumeMultiplier(FMath::Max(HeartbeatVolume, 0.0f));
	HeartbeatComponent->bAutoActivate = false;
	HeartbeatComponent->bAllowSpatialization = false;
	HeartbeatComponent->FadeIn(FMath::Max(HeartbeatFadeIn, 0.0f), FMath::Max(HeartbeatVolume, 0.0f));
}

void AOBPanicAudioManager::StopHeartbeatLayer()
{
	bHeartbeatLayerActive = false;
	if (HeartbeatComponent && HeartbeatComponent->IsPlaying())
	{
		HeartbeatComponent->FadeOut(FMath::Max(HeartbeatFadeOut, 0.0f), 0.0f);
	}
}

void AOBPanicAudioManager::StartCrowdRoarLayer()
{
	StopCrowdRoarLayer();

	switch (CrowdRoarMode)
	{
	case ECrowdRoarMode::RandomSingle:
		{
			TArray<TTuple<USoundBase*, float, const TCHAR*>> Roars;
			if (CrowdRoar1) { Roars.Emplace(CrowdRoar1, Roar1Volume, TEXT("CrowdRoar1")); }
			if (CrowdRoar2) { Roars.Emplace(CrowdRoar2, Roar2Volume, TEXT("CrowdRoar2")); }
			if (CrowdRoar3) { Roars.Emplace(CrowdRoar3, Roar3Volume, TEXT("CrowdRoar3")); }
			if (Roars.Num() == 0)
			{
				DebugMissingSound(TEXT("CrowdRoar1/CrowdRoar2/CrowdRoar3"));
				return;
			}
			const TTuple<USoundBase*, float, const TCHAR*>& Selected = Roars[FMath::RandRange(0, Roars.Num() - 1)];
			PlayRoarSound(Selected.Get<0>(), Selected.Get<1>(), Selected.Get<2>());
			break;
		}
	case ECrowdRoarMode::LayeredPreset:
		if (CrowdRoarPreset == ECrowdLayeredRoarPreset::Roar1AndRoar2 || CrowdRoarPreset == ECrowdLayeredRoarPreset::Roar1AndRoar3 || CrowdRoarPreset == ECrowdLayeredRoarPreset::Roar1Roar2Roar3)
		{
			PlayRoarSound(CrowdRoar1, Roar1Volume, TEXT("CrowdRoar1"));
		}
		if (CrowdRoarPreset == ECrowdLayeredRoarPreset::Roar1AndRoar2 || CrowdRoarPreset == ECrowdLayeredRoarPreset::Roar1Roar2Roar3)
		{
			PlayRoarSound(CrowdRoar2, Roar2Volume, TEXT("CrowdRoar2"));
		}
		if (CrowdRoarPreset == ECrowdLayeredRoarPreset::Roar1AndRoar3 || CrowdRoarPreset == ECrowdLayeredRoarPreset::Roar1Roar2Roar3)
		{
			PlayRoarSound(CrowdRoar3, Roar3Volume, TEXT("CrowdRoar3"));
		}
		break;
	case ECrowdRoarMode::CustomMix:
		if (bCustomUseRoar1) { PlayRoarSound(CrowdRoar1, Roar1Volume, TEXT("CrowdRoar1")); }
		if (bCustomUseRoar2) { PlayRoarSound(CrowdRoar2, Roar2Volume, TEXT("CrowdRoar2")); }
		if (bCustomUseRoar3) { PlayRoarSound(CrowdRoar3, Roar3Volume, TEXT("CrowdRoar3")); }
		break;
	}
}

void AOBPanicAudioManager::StopCrowdRoarLayer()
{
	for (UAudioComponent* Component : ActiveRoarComponents)
	{
		FadeOutAndForget(Component, RoarFadeOut);
	}
	ActiveRoarComponents.Reset();
}

void AOBPanicAudioManager::StartCrowdFootstepsLayer()
{
	StopCrowdFootstepsLayer();

	switch (CrowdFootstepMode)
	{
	case ECrowdFootstepMode::Run1Only:
		PlayFootstepSound(CrowdFootstepRun1, FootstepRun1Volume, TEXT("CrowdFootstepRun1"));
		break;
	case ECrowdFootstepMode::Run2Only:
		PlayFootstepSound(CrowdFootstepRun2, FootstepRun2Volume, TEXT("CrowdFootstepRun2"));
		break;
	case ECrowdFootstepMode::Run1AndRun2:
		PlayFootstepSound(CrowdFootstepRun1, FootstepRun1Volume, TEXT("CrowdFootstepRun1"));
		PlayFootstepSound(CrowdFootstepRun2, FootstepRun2Volume, TEXT("CrowdFootstepRun2"));
		break;
	case ECrowdFootstepMode::CustomMix:
		if (bCustomUseFootstepRun1) { PlayFootstepSound(CrowdFootstepRun1, FootstepRun1Volume, TEXT("CrowdFootstepRun1")); }
		if (bCustomUseFootstepRun2) { PlayFootstepSound(CrowdFootstepRun2, FootstepRun2Volume, TEXT("CrowdFootstepRun2")); }
		break;
	}
}

void AOBPanicAudioManager::StopCrowdFootstepsLayer()
{
	for (UAudioComponent* Component : ActiveFootstepComponents)
	{
		FadeOutAndForget(Component, FootstepsFadeOut);
	}
	ActiveFootstepComponents.Reset();
}

void AOBPanicAudioManager::StopLowDroneLayer()
{
	if (LowDroneComponent && LowDroneComponent->IsPlaying())
	{
		LowDroneComponent->FadeOut(FMath::Max(PickupReliefFadeOut, 0.0f), 0.0f);
	}
}

void AOBPanicAudioManager::KeepLoopLayersAlive()
{
	if (bHeartbeatLayerActive && HeartbeatComponent && HeartbeatSound && !HeartbeatComponent->IsPlaying())
	{
		HeartbeatComponent->SetSound(HeartbeatSound);
		HeartbeatComponent->FadeIn(0.0f, FMath::Max(HeartbeatVolume, 0.0f));
	}

	for (UAudioComponent* Component : ActiveFootstepComponents)
	{
		if (Component && Component->Sound && !Component->IsPlaying())
		{
			Component->FadeIn(0.0f, Component->VolumeMultiplier);
		}
	}
}

void AOBPanicAudioManager::ScheduleNextAmbientHorror()
{
	if (UWorld* World = GetWorld())
	{
		const float Delay = FMath::FRandRange(AmbientMinInterval, AmbientMaxInterval);
		World->GetTimerManager().SetTimer(AmbientHorrorTimerHandle, this, &AOBPanicAudioManager::PlayRandomAmbientHorror, Delay, false);
	}
}

void AOBPanicAudioManager::PlayAmbientSound(USoundBase* Sound, float Volume, const TCHAR* DebugName)
{
	if (!Sound)
	{
		DebugMissingSound(DebugName);
		return;
	}

	if (UAudioComponent* Component = CreateAudioLayer(Sound, Volume, 1.0f, DebugName))
	{
		ActiveAmbientComponents.Add(Component);
		Component->FadeIn(FMath::Max(AmbientFadeIn, 0.0f), FMath::Max(Volume, 0.0f));
		if (UWorld* World = GetWorld())
		{
			const float Duration = Sound->GetDuration();
			if (Duration > 0.0f && Duration < 1000000.0f)
			{
				FTimerHandle AmbientFadeTimerHandle;
				const float FadeDelay = FMath::Max(Duration - FMath::Max(AmbientFadeOut, 0.0f), 0.0f);
				World->GetTimerManager().SetTimer(
					AmbientFadeTimerHandle,
					FTimerDelegate::CreateWeakLambda(this, [this, Component]()
					{
						FadeOutAndForget(Component, AmbientFadeOut);
					}),
					FadeDelay,
					false);
			}
		}
	}
}

void AOBPanicAudioManager::PlayRoarSound(USoundBase* Sound, float Volume, const TCHAR* DebugName)
{
	if (UAudioComponent* Component = CreateAudioLayer(Sound, Volume, 1.0f, DebugName))
	{
		ActiveRoarComponents.Add(Component);
		Component->FadeIn(FMath::Max(RoarFadeIn, 0.0f), FMath::Max(Volume, 0.0f));
	}
}

void AOBPanicAudioManager::PlayFootstepSound(USoundBase* Sound, float Volume, const TCHAR* DebugName)
{
	if (UAudioComponent* Component = CreateAudioLayer(Sound, Volume, 1.0f, DebugName))
	{
		ActiveFootstepComponents.Add(Component);
		Component->FadeIn(FMath::Max(FootstepsFadeIn, 0.0f), FMath::Max(Volume, 0.0f));
	}
}

UAudioComponent* AOBPanicAudioManager::CreateAudioLayer(USoundBase* Sound, float Volume, float Pitch, const TCHAR* DebugName)
{
	if (!Sound)
	{
		DebugMissingSound(DebugName);
		return nullptr;
	}

	UAudioComponent* Component = UGameplayStatics::CreateSound2D(
		this,
		Sound,
		FMath::Max(Volume, 0.0f),
		FMath::Max(Pitch, 0.1f),
		0.0f,
		nullptr,
		false,
		false);
	if (!Component)
	{
		return nullptr;
	}

	Component->bAllowSpatialization = false;
	Component->bAutoActivate = false;
	Component->bAutoDestroy = false;
	return Component;
}

void AOBPanicAudioManager::FadeOutAndForget(UAudioComponent* Component, float FadeOutDuration) const
{
	if (!Component)
	{
		return;
	}

	if (Component->IsPlaying())
	{
		Component->bAutoDestroy = true;
		Component->FadeOut(FMath::Max(FadeOutDuration, 0.0f), 0.0f);
	}
}

void AOBPanicAudioManager::DebugMissingSound(const TCHAR* SoundName) const
{
	UE_LOG(LogOBPanicAudio, Verbose, TEXT("Missing Sound Asset: %s"), SoundName);
}
