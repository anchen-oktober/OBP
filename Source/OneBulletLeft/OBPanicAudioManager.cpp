#include "OBPanicAudioManager.h"

#include "AIController.h"
#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "OBEnemy.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
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
	StartAudioBalanceTestMode();
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
	if (bEnableAudioDebugLogs)
	{
		PrintAudioDebug(TEXT("Exit Panic State"));
	}
	RestoreMusicAfterPanic();
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
		SelectedMusicSound = nullptr;
		if (bEnableAudioDebugLogs)
		{
			UE_LOG(LogOBPanicAudio, Log, TEXT("Background music disabled: bMusicEnabled is false"));
		}
		return;
	}

	if (BackgroundMusicComponent && BackgroundMusicComponent->IsPlaying())
	{
		return;
	}

	USoundBase* MusicToPlay = SelectedMusicSound.Get();
	if (!MusicToPlay)
	{
		MusicToPlay = SelectBackgroundMusicTrack();
		SelectedMusicSound = MusicToPlay;
	}

	if (!MusicToPlay)
	{
		ReportAudioWarning(TEXT("Background music skipped: BackgroundMusicTracks is empty."));
		return;
	}

	const float TargetMusicVolume = bPanicAudioActive && bEnableMusicDucking ? GetPanicMusicVolume() : GetBaseMusicVolume();
	if (FadeInExistingLayer(BackgroundMusicComponent, MusicToPlay, TargetMusicVolume, MusicFadeIn, TEXT("Music")))
	{
		return;
	}

	BackgroundMusicComponent = CreateAudioLayer(MusicToPlay, TargetMusicVolume, 1.0f, TEXT("BackgroundMusic"));
	if (BackgroundMusicComponent)
	{
		LogAudioLayerStarted(TEXT("Music"), MusicToPlay, TargetMusicVolume);
		BackgroundMusicComponent->FadeIn(FMath::Max(MusicFadeIn, 0.0f), TargetMusicVolume);
		if (bEnableAudioDebugLogs)
		{
			PrintAudioDebug(FString::Printf(TEXT("Start Music + volume %.2f | started=true | LoopMusic=%s"), TargetMusicVolume, bLoopMusic ? TEXT("true") : TEXT("false")));
		}
	}
}

void AOBPanicAudioManager::StopMusicLayer()
{
	if (BackgroundMusicComponent && BackgroundMusicComponent->IsPlaying())
	{
		FadeOutReusableLayer(BackgroundMusicComponent, MusicFadeOut);
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

		Enemy->ApplyBulletPickupReliefReaction(
			Player,
			FMath::Max(FearDuration, 0.0f),
			FMath::Clamp(FearSpeedMultiplier, 0.0f, 1.0f),
			FMath::Max(RetreatDistanceMin, 0.0f),
			FMath::Max(RetreatDistanceMax, RetreatDistanceMin),
			FMath::Max(RetreatDurationMin, 0.01f),
			FMath::Max(RetreatDurationMax, RetreatDurationMin),
			FMath::Max(RetreatAngleVariation, 0.0f),
			FMath::Max(ExtraSafeDistance, 0.0f),
			FMath::Max(ReactionDelayMin, 0.0f),
			FMath::Max(ReactionDelayMax, ReactionDelayMin),
			FMath::Clamp(ReactionPauseSpeedMultiplierMin, 0.0f, 1.0f),
			FMath::Clamp(ReactionPauseSpeedMultiplierMax, ReactionPauseSpeedMultiplierMin, 1.0f));
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
	if (bEnableAudioDebugLogs)
	{
		PrintAudioDebug(TEXT("Enter Panic State"));
	}
	DuckMusicForPanic();
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
	if (!IsLayerVolumeAudible(TEXT("Heartbeat"), HeartbeatVolume))
	{
		return;
	}

	if (FadeInExistingLayer(HeartbeatComponent, HeartbeatSound, HeartbeatVolume, HeartbeatFadeIn, TEXT("Heartbeat")))
	{
		return;
	}

	HeartbeatComponent = CreateAudioLayer(HeartbeatSound, HeartbeatVolume, 1.0f, TEXT("HeartbeatSound"));
	if (HeartbeatComponent)
	{
		LogAudioLayerStarted(TEXT("Heartbeat"), HeartbeatSound, HeartbeatVolume);
		HeartbeatComponent->FadeIn(FMath::Max(HeartbeatFadeIn, 0.0f), FMath::Max(HeartbeatVolume, 0.0f));
	}
}

void AOBPanicAudioManager::StopHeartbeatLayer()
{
	bHeartbeatLayerActive = false;
	if (HeartbeatComponent && HeartbeatComponent->IsPlaying())
	{
		FadeOutReusableLayer(HeartbeatComponent, HeartbeatFadeOut);
	}
}

void AOBPanicAudioManager::StartLowDroneLayer()
{
	if (!LowDroneSound)
	{
		DebugMissingSound(TEXT("LowDroneSound"));
		return;
	}
	if (!IsLayerVolumeAudible(TEXT("Low Drone"), LowDroneVolume))
	{
		return;
	}

	if (FadeInExistingLayer(LowDroneComponent, LowDroneSound, LowDroneVolume, LowDroneFadeIn, TEXT("Low Drone")))
	{
		return;
	}

	LowDroneComponent = CreateAudioLayer(LowDroneSound, LowDroneVolume, 1.0f, TEXT("LowDroneSound"));
	if (LowDroneComponent)
	{
		LogAudioLayerStarted(TEXT("Low Drone"), LowDroneSound, LowDroneVolume);
		LowDroneComponent->FadeIn(FMath::Max(LowDroneFadeIn, 0.0f), FMath::Max(LowDroneVolume, 0.0f));
	}
}

void AOBPanicAudioManager::StartCrowdRoarLayer()
{
	bRoarLayerActive = true;

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
	bRoarLayerActive = false;
	for (UAudioComponent* Component : ActiveRoarComponents)
	{
		FadeOutReusableLayer(Component, RoarFadeOut);
	}
}

void AOBPanicAudioManager::StartCrowdFootstepsLayer()
{
	bFootstepsLayerActive = true;

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
	bFootstepsLayerActive = false;
	for (UAudioComponent* Component : ActiveFootstepComponents)
	{
		FadeOutReusableLayer(Component, FootstepsFadeOut);
	}
}

void AOBPanicAudioManager::StopLowDroneLayer()
{
	if (LowDroneComponent && LowDroneComponent->IsPlaying())
	{
		const float FadeOut = FMath::Max(LowDroneFadeOut, PickupReliefFadeOut);
		FadeOutReusableLayer(LowDroneComponent, FadeOut);
	}
}

void AOBPanicAudioManager::KeepLoopLayersAlive()
{
	if (bMusicEnabled && bLoopMusic && BackgroundMusicComponent && SelectedMusicSound && !BackgroundMusicComponent->IsPlaying())
	{
		BackgroundMusicComponent->SetSound(SelectedMusicSound);
		const float TargetMusicVolume = bPanicAudioActive && bEnableMusicDucking ? GetPanicMusicVolume() : GetBaseMusicVolume();
		LogAudioLayerStarted(TEXT("Music Restart"), SelectedMusicSound, TargetMusicVolume);
		BackgroundMusicComponent->FadeIn(FMath::Max(MusicFadeIn, 0.0f), TargetMusicVolume);
	}

	if (bHeartbeatLayerActive && HeartbeatComponent && HeartbeatSound && !HeartbeatComponent->IsPlaying())
	{
		HeartbeatComponent->SetSound(HeartbeatSound);
		HeartbeatComponent->FadeIn(FMath::Max(HeartbeatFadeIn, 0.0f), FMath::Max(HeartbeatVolume, 0.0f));
	}

	if (bPanicAudioActive && LowDroneComponent && LowDroneSound && !LowDroneComponent->IsPlaying())
	{
		LowDroneComponent->SetSound(LowDroneSound);
		LowDroneComponent->FadeIn(FMath::Max(LowDroneFadeIn, 0.0f), FMath::Max(LowDroneVolume, 0.0f));
	}

	for (UAudioComponent* Component : ActiveRoarComponents)
	{
		if (bPanicAudioActive && bRoarLayerActive && Component && Component->Sound && !Component->IsPlaying())
		{
			Component->FadeIn(FMath::Max(RoarFadeIn, 0.0f), FMath::Max(Component->VolumeMultiplier, 0.0f));
		}
	}

	for (UAudioComponent* Component : ActiveFootstepComponents)
	{
		if (bPanicAudioActive && bFootstepsLayerActive && Component && Component->Sound && !Component->IsPlaying())
		{
			Component->FadeIn(FMath::Max(FootstepsFadeIn, 0.0f), FMath::Max(Component->VolumeMultiplier, 0.0f));
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
		LogAudioLayerStarted(TEXT("Ambient Horror"), Sound, Volume);
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
	if (!IsLayerVolumeAudible(DebugName, Volume))
	{
		return;
	}

	if (FadeInExistingLayer(FindReusableLayerComponent(ActiveRoarComponents, Sound), Sound, Volume, RoarFadeIn, TEXT("Roar")))
	{
		return;
	}

	if (UAudioComponent* Component = CreateAudioLayer(Sound, Volume, 1.0f, DebugName))
	{
		ActiveRoarComponents.Add(Component);
		LogAudioLayerStarted(TEXT("Roar"), Sound, Volume);
		Component->FadeIn(FMath::Max(RoarFadeIn, 0.0f), FMath::Max(Volume, 0.0f));
	}
}

void AOBPanicAudioManager::PlayFootstepSound(USoundBase* Sound, float Volume, const TCHAR* DebugName)
{
	if (!IsLayerVolumeAudible(DebugName, Volume))
	{
		return;
	}

	if (FadeInExistingLayer(FindReusableLayerComponent(ActiveFootstepComponents, Sound), Sound, Volume, FootstepsFadeIn, TEXT("Footsteps")))
	{
		return;
	}

	if (UAudioComponent* Component = CreateAudioLayer(Sound, Volume, 1.0f, DebugName))
	{
		ActiveFootstepComponents.Add(Component);
		LogAudioLayerStarted(TEXT("Footsteps"), Sound, Volume);
		Component->FadeIn(FMath::Max(FootstepsFadeIn, 0.0f), FMath::Max(Volume, 0.0f));
	}
}

USoundBase* AOBPanicAudioManager::SelectBackgroundMusicTrack()
{
	if (BackgroundMusicTracks.Num() == 0)
	{
		return nullptr;
	}

	if (BackgroundMusicTracks.Num() > 5)
	{
		ReportAudioWarning(TEXT("BP_PanicAudioManager supports max 5 background music tracks."));
	}

	TArray<USoundBase*> ValidTracks;
	const int32 MaxTrackCount = FMath::Min(BackgroundMusicTracks.Num(), 5);
	for (int32 TrackIndex = 0; TrackIndex < MaxTrackCount; ++TrackIndex)
	{
		if (USoundBase* Track = BackgroundMusicTracks[TrackIndex].Get())
		{
			ValidTracks.Add(Track);
		}
	}

	if (ValidTracks.Num() == 0)
	{
		ReportAudioWarning(TEXT("Background music skipped: first 5 BackgroundMusicTracks entries are empty."));
		return nullptr;
	}

	USoundBase* SelectedTrack = ValidTracks[0];
	if (bRandomizeMusicOnStart && ValidTracks.Num() > 1)
	{
		SelectedTrack = ValidTracks[FMath::RandRange(0, ValidTracks.Num() - 1)];
	}

	if (bEnableAudioDebugLogs)
	{
		UE_LOG(
			LogOBPanicAudio,
			Log,
			TEXT("Background music tracks found: %d | Selected music track: %s | MusicVolume: %.2f"),
			ValidTracks.Num(),
			*GetNameSafe(SelectedTrack),
			GetBaseMusicVolume());
	}

	return SelectedTrack;
}

float AOBPanicAudioManager::GetBaseMusicVolume() const
{
	return FMath::Max(NormalMusicVolume, 0.0f);
}

float AOBPanicAudioManager::GetPanicMusicVolume() const
{
	return FMath::Max(PanicMusicVolume, 0.0f);
}

void AOBPanicAudioManager::SetBackgroundMusicVolume(float TargetVolume, float FadeTime, const TCHAR* DebugReason)
{
	if (!BackgroundMusicComponent)
	{
		return;
	}

	const float ClampedVolume = FMath::Max(TargetVolume, 0.0f);
	const float ClampedFadeTime = FMath::Max(FadeTime, 0.0f);
	if (BackgroundMusicComponent->IsPlaying())
	{
		BackgroundMusicComponent->AdjustVolume(ClampedFadeTime, ClampedVolume);
	}
	else
	{
		BackgroundMusicComponent->SetVolumeMultiplier(ClampedVolume);
	}

	if (bEnableAudioDebugLogs)
	{
		PrintAudioDebug(FString::Printf(TEXT("%s %.2f"), DebugReason, ClampedVolume));
	}
}

void AOBPanicAudioManager::DuckMusicForPanic()
{
	if (!bEnableMusicDucking)
	{
		return;
	}

	SetBackgroundMusicVolume(GetPanicMusicVolume(), MusicDuckFadeTime, TEXT("Music ducked to PanicMusicVolume"));
}

void AOBPanicAudioManager::RestoreMusicAfterPanic()
{
	if (!bEnableMusicDucking)
	{
		return;
	}

	SetBackgroundMusicVolume(GetBaseMusicVolume(), MusicRestoreFadeTime, TEXT("Music restored to NormalMusicVolume"));
}

bool AOBPanicAudioManager::IsLayerVolumeAudible(const TCHAR* LayerName, float Volume) const
{
	if (Volume > 0.0f)
	{
		return true;
	}

	ReportAudioWarning(FString::Printf(TEXT("%s skipped: volume is 0."), LayerName));
	return false;
}

void AOBPanicAudioManager::StartAudioBalanceTestMode()
{
	if (!bAudioBalanceTestMode)
	{
		return;
	}

	AudioBalanceTestStep = 0;
	PrintAudioDebug(TEXT("Audio Balance Test Mode started"));
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AudioBalanceTestTimerHandle, this, &AOBPanicAudioManager::PlayAudioBalanceTestStep, 1.0f, true, 0.0f);
	}
}

void AOBPanicAudioManager::PlayAudioBalanceTestStep()
{
	switch (AudioBalanceTestStep)
	{
	case 0:
		PrintAudioDebug(FString::Printf(TEXT("Audio Balance Test: Music volume %.2f"), GetBaseMusicVolume()));
		StartMusicLayer();
		break;
	case 1:
		PrintAudioDebug(FString::Printf(TEXT("Audio Balance Test: Heartbeat volume %.2f"), HeartbeatVolume));
		StartHeartbeatLayer();
		break;
	case 2:
		PrintAudioDebug(FString::Printf(TEXT("Audio Balance Test: Roar volumes %.2f / %.2f / %.2f"), Roar1Volume, Roar2Volume, Roar3Volume));
		StartCrowdRoarLayer();
		break;
	case 3:
		PrintAudioDebug(FString::Printf(TEXT("Audio Balance Test: Footsteps volumes %.2f / %.2f"), FootstepRun1Volume, FootstepRun2Volume));
		StartCrowdFootstepsLayer();
		break;
	case 4:
		PrintAudioDebug(FString::Printf(TEXT("Audio Balance Test: Low Drone volume %.2f"), LowDroneVolume));
		StartLowDroneLayer();
		break;
	default:
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(AudioBalanceTestTimerHandle);
		}
		PrintAudioDebug(TEXT("Audio Balance Test Mode finished"));
		return;
	}

	++AudioBalanceTestStep;
}

void AOBPanicAudioManager::PrintAudioDebug(const FString& Message) const
{
	UE_LOG(LogOBPanicAudio, Log, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, Message);
	}
}

void AOBPanicAudioManager::ReportAudioWarning(const FString& Message) const
{
	UE_LOG(LogOBPanicAudio, Warning, TEXT("%s"), *Message);
	if (bEnableAudioDebugLogs && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Message);
	}
}

void AOBPanicAudioManager::LogAudioLayerStarted(const TCHAR* LayerName, USoundBase* Sound, float Volume) const
{
	if (!bEnableAudioDebugLogs)
	{
		return;
	}

	FString SoundClassName = TEXT("None");
	if (Sound && Sound->GetSoundClass())
	{
		SoundClassName = Sound->GetSoundClass()->GetName();
	}

	UE_LOG(
		LogOBPanicAudio,
		Log,
		TEXT("Start %s | Sound: %s | Volume: %.2f | SoundClass: %s"),
		LayerName,
		*GetNameSafe(Sound),
		FMath::Max(Volume, 0.0f),
		*SoundClassName);
}

UAudioComponent* AOBPanicAudioManager::CreateAudioLayer(USoundBase* Sound, float Volume, float Pitch, const TCHAR* DebugName)
{
	if (!Sound)
	{
		DebugMissingSound(DebugName);
		return nullptr;
	}

	UAudioComponent* Component = UGameplayStatics::SpawnSound2D(
		this,
		Sound,
		0.0f,
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
	Component->bAutoDestroy = false;
	Component->SetVolumeMultiplier(FMath::Max(Volume, 0.0f));
	return Component;
}

UAudioComponent* AOBPanicAudioManager::FindReusableLayerComponent(TArray<TObjectPtr<UAudioComponent>>& Components, USoundBase* Sound) const
{
	if (!Sound)
	{
		return nullptr;
	}

	for (int32 Index = Components.Num() - 1; Index >= 0; --Index)
	{
		UAudioComponent* Component = Components[Index].Get();
		if (!Component)
		{
			Components.RemoveAtSwap(Index);
			continue;
		}

		if (Component->Sound == Sound)
		{
			return Component;
		}
	}

	return nullptr;
}

bool AOBPanicAudioManager::FadeInExistingLayer(UAudioComponent* Component, USoundBase* Sound, float Volume, float FadeInDuration, const TCHAR* LayerName)
{
	if (!Component || !Sound)
	{
		return false;
	}

	Component->bAutoDestroy = false;
	Component->SetSound(Sound);
	Component->SetVolumeMultiplier(FMath::Max(Volume, 0.0f));
	LogAudioLayerStarted(LayerName, Sound, Volume);
	Component->FadeIn(FMath::Max(FadeInDuration, 0.0f), FMath::Max(Volume, 0.0f));
	return true;
}

void AOBPanicAudioManager::FadeOutReusableLayer(UAudioComponent* Component, float FadeOutDuration) const
{
	if (!Component || !Component->IsPlaying())
	{
		return;
	}

	Component->bAutoDestroy = false;
	Component->FadeOut(FMath::Max(FadeOutDuration, 0.0f), 0.0f);
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
