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
}

void AOBPanicAudioManager::BeginPlay()
{
	Super::BeginPlay();
	StartMusicLayer();
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
		World->GetTimerManager().ClearTimer(LowDroneStartTimerHandle);
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
	if (!bAmbientHorrorEnabled)
	{
		return;
	}

	ScheduleNextAmbientHorror();
}

void AOBPanicAudioManager::StopAmbientHorrorLoop()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AmbientHorrorTimerHandle);
	}

	for (UAudioComponent* Component : ActiveAmbientComponents)
	{
		FadeOutAndForget(Component, AmbientFadeOut);
	}
	ActiveAmbientComponents.Reset();
}

void AOBPanicAudioManager::StartPanicSequence(AActor* AudioFocus)
{
	StartPanicAudio(AudioFocus);
}

void AOBPanicAudioManager::StopPanicSequence(AActor* AudioFocus, bool bPlayBulletPickupSound)
{
	StopPanicAudio(AudioFocus, bPlayBulletPickupSound);
}

void AOBPanicAudioManager::PlayRoar()
{
	StartCrowdRoarLayer();
}

void AOBPanicAudioManager::PlayFootsteps()
{
	StartCrowdFootstepsLayer();
}

void AOBPanicAudioManager::StartMusicLayer()
{
	if (!bMusicEnabled)
	{
		return;
	}

	if (MusicComponent && MusicComponent->IsPlaying())
	{
		return;
	}

	if (!MusicSound)
	{
		DebugMissingSound(TEXT("MusicSound"));
		return;
	}

	FadeOutAndForget(MusicComponent, 0.0f);
	MusicComponent = CreateAudioLayer(MusicSound, MusicVolume, 1.0f, TEXT("MusicSound"));
	if (MusicComponent)
	{
		MusicComponent->FadeIn(FMath::Max(MusicFadeIn, 0.0f), FMath::Max(MusicVolume, 0.0f));
	}
}

void AOBPanicAudioManager::StopMusicLayer()
{
	if (MusicComponent && MusicComponent->IsPlaying())
	{
		MusicComponent->FadeOut(FMath::Max(MusicFadeOut, 0.0f), 0.0f);
	}
}

void AOBPanicAudioManager::TryPlayAmbientHorror()
{
	if (!bAmbientHorrorEnabled)
	{
		return;
	}

	const float Roll = FMath::FRand();
	if (Roll > FMath::Clamp(AmbientPlayChance, 0.0f, 1.0f))
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

void AOBPanicAudioManager::PlayRandomAmbientHorror()
{
	TryPlayAmbientHorror();
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
		const float RoarDelay = LowDroneDelayAfterHeartbeat + RoarDelayAfterLowDrone;
		World->GetTimerManager().SetTimer(LowDroneStartTimerHandle, this, &AOBPanicAudioManager::StartLowDroneLayer, LowDroneDelayAfterHeartbeat, false);
		World->GetTimerManager().SetTimer(RoarStartTimerHandle, this, &AOBPanicAudioManager::StartCrowdRoarLayer, RoarDelay, false);
		World->GetTimerManager().SetTimer(FootstepsStartTimerHandle, this, &AOBPanicAudioManager::StartCrowdFootstepsLayer, RoarDelay + FootstepsDelayAfterRoar, false);
	}
}

void AOBPanicAudioManager::StartHeartbeatLayer()
{
	bHeartbeatLayerActive = true;
	if (!HeartbeatSound)
	{
		DebugMissingSound(TEXT("HeartbeatSound"));
		return;
	}

	FadeOutAndForget(HeartbeatComponent, 0.0f);
	HeartbeatComponent = CreateAudioLayer(HeartbeatSound, HeartbeatVolume, 1.0f, TEXT("HeartbeatSound"));
	if (HeartbeatComponent)
	{
		HeartbeatComponent->FadeIn(FMath::Max(HeartbeatFadeIn, 0.0f), FMath::Max(HeartbeatVolume, 0.0f));
	}
}

void AOBPanicAudioManager::StopHeartbeatLayer()
{
	bHeartbeatLayerActive = false;
	if (HeartbeatComponent && HeartbeatComponent->IsPlaying())
	{
		HeartbeatComponent->FadeOut(FMath::Max(HeartbeatFadeOut, 0.0f), 0.0f);
	}
}

void AOBPanicAudioManager::StartLowDroneLayer()
{
	if (!LowDroneSound)
	{
		DebugMissingSound(TEXT("LowDroneSound"));
		return;
	}

	FadeOutAndForget(LowDroneComponent, 0.0f);
	LowDroneComponent = CreateAudioLayer(LowDroneSound, LowDroneVolume, 1.0f, TEXT("LowDroneSound"));
	if (LowDroneComponent)
	{
		LowDroneComponent->FadeIn(FMath::Max(LowDroneFadeIn, 0.0f), FMath::Max(LowDroneVolume, 0.0f));
	}
}

void AOBPanicAudioManager::StartCrowdRoarLayer()
{
	StopCrowdRoarLayer();

	switch (RoarMode)
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
	case ECrowdRoarMode::PresetCombination:
		if (RoarPreset == ECrowdRoarPreset::Roar1AndRoar2 || RoarPreset == ECrowdRoarPreset::Roar1AndRoar3 || RoarPreset == ECrowdRoarPreset::Roar1AndRoar2AndRoar3)
		{
			PlayRoarSound(CrowdRoar1, Roar1Volume, TEXT("CrowdRoar1"));
		}
		if (RoarPreset == ECrowdRoarPreset::Roar1AndRoar2 || RoarPreset == ECrowdRoarPreset::Roar2AndRoar3 || RoarPreset == ECrowdRoarPreset::Roar1AndRoar2AndRoar3)
		{
			PlayRoarSound(CrowdRoar2, Roar2Volume, TEXT("CrowdRoar2"));
		}
		if (RoarPreset == ECrowdRoarPreset::Roar1AndRoar3 || RoarPreset == ECrowdRoarPreset::Roar2AndRoar3 || RoarPreset == ECrowdRoarPreset::Roar1AndRoar2AndRoar3)
		{
			PlayRoarSound(CrowdRoar3, Roar3Volume, TEXT("CrowdRoar3"));
		}
		break;
	case ECrowdRoarMode::CustomMix:
		if (bRoar1Enabled) { PlayRoarSound(CrowdRoar1, Roar1Volume, TEXT("CrowdRoar1")); }
		if (bRoar2Enabled) { PlayRoarSound(CrowdRoar2, Roar2Volume, TEXT("CrowdRoar2")); }
		if (bRoar3Enabled) { PlayRoarSound(CrowdRoar3, Roar3Volume, TEXT("CrowdRoar3")); }
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

	switch (FootstepsMode)
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
		const float FadeOut = FMath::Max(LowDroneFadeOut, PickupReliefFadeOut);
		LowDroneComponent->FadeOut(FadeOut, 0.0f);
	}
}

void AOBPanicAudioManager::KeepLoopLayersAlive()
{
	if (bMusicEnabled && MusicComponent && MusicSound && !MusicComponent->IsPlaying())
	{
		MusicComponent->SetSound(MusicSound);
		MusicComponent->FadeIn(0.0f, FMath::Max(MusicVolume, 0.0f));
	}

	if (bHeartbeatLayerActive && HeartbeatComponent && HeartbeatSound && !HeartbeatComponent->IsPlaying())
	{
		HeartbeatComponent->SetSound(HeartbeatSound);
		HeartbeatComponent->FadeIn(0.0f, FMath::Max(HeartbeatVolume, 0.0f));
	}

	if (bPanicAudioActive && LowDroneComponent && LowDroneSound && !LowDroneComponent->IsPlaying())
	{
		LowDroneComponent->SetSound(LowDroneSound);
		LowDroneComponent->FadeIn(0.0f, FMath::Max(LowDroneVolume, 0.0f));
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
	if (!bAmbientHorrorEnabled)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const float MinInterval = FMath::Max(MinAmbientInterval, 1.0f);
		const float MaxInterval = FMath::Max(MaxAmbientInterval, MinInterval);
		const float Delay = FMath::FRandRange(MinInterval, MaxInterval);
		World->GetTimerManager().SetTimer(AmbientHorrorTimerHandle, this, &AOBPanicAudioManager::TryPlayAmbientHorror, Delay, false);
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
