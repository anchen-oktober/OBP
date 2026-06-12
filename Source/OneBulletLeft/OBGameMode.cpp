#include "OBGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "OBBulletPickup.h"
#include "OBCharacter.h"
#include "OBGameState.h"
#include "OBHUD.h"

AOBGameMode::AOBGameMode()
{
	DefaultPawnClass = AOBCharacter::StaticClass();
	GameStateClass = AOBGameState::StaticClass();
	HUDClass = AOBHUD::StaticClass();
	BulletPickupClass = AOBBulletPickup::StaticClass();
	WaveManagerClass = AOBWaveManager::StaticClass();
}

void AOBGameMode::BeginPlay()
{
	Super::BeginPlay();
	ApplyWindowMode();
	GetWorldTimerManager().SetTimer(WindowModeTimerHandle, this, &AOBGameMode::ApplyWindowMode, 0.25f, false);

	if (AOBGameState* OneBulletState = GetGameState<AOBGameState>())
	{
		OneBulletState->SetBulletReady(true);
		OneBulletState->SetGameOver(false);
	}

	InitializeWaveManager();
}

void AOBGameMode::ApplyWindowMode()
{
	if (!bForceWindowedMode)
	{
		return;
	}

	UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!Settings)
	{
		return;
	}

	Settings->SetFullscreenMode(EWindowMode::Windowed);
	Settings->SetScreenResolution(FIntPoint(WindowedResolutionX, WindowedResolutionY));
	Settings->ApplySettings(false);
	Settings->SaveSettings();

	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		PlayerController->ConsoleCommand(FString::Printf(TEXT("r.setres %dx%dw"), WindowedResolutionX, WindowedResolutionY), true);
	}
	else if (GEngine && GetWorld())
	{
		GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.setres %dx%dw"), WindowedResolutionX, WindowedResolutionY));
	}
}

void AOBGameMode::InitializeWaveManager()
{
	for (TActorIterator<AOBWaveManager> It(GetWorld()); It; ++It)
	{
		WaveManager = *It;
		break;
	}

	if (!WaveManager)
	{
		TSubclassOf<AOBWaveManager> ClassToSpawn = WaveManagerClass;
		if (!ClassToSpawn)
		{
			ClassToSpawn = AOBWaveManager::StaticClass();
		}
		WaveManager = GetWorld()->SpawnActor<AOBWaveManager>(ClassToSpawn);
	}

	if (!WaveManager)
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to create WaveManager"));
		return;
	}

}

AOBBulletPickup* AOBGameMode::SpawnBulletPickup(const FVector& DropLocation)
{
	if (!BulletPickupClass)
	{
		return nullptr;
	}

	FVector SpawnLocation = DropLocation;
	SpawnLocation.Z += BulletPickupDropHeight;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	return GetWorld()->SpawnActor<AOBBulletPickup>(BulletPickupClass, SpawnLocation, FRotator::ZeroRotator, Params);
}

void AOBGameMode::PlayBulletTrail(const FVector& TraceStart, const FVector& TraceEnd)
{
	PlayBulletFlight(TraceStart, TraceEnd, nullptr);
}

void AOBGameMode::PlayBulletFlight(const FVector& TraceStart, const FVector& TraceEnd, AOBBulletPickup* DestinationPickup)
{
	if (BulletPickupClass)
	{
		const AOBBulletPickup* PickupDefaults = BulletPickupClass->GetDefaultObject<AOBBulletPickup>();
		if (PickupDefaults)
		{
			PickupDefaults->PlayTrailEffectToPickup(this, TraceStart, TraceEnd, DestinationPickup);
		}
	}
}

void AOBGameMode::RestartRun(AOBCharacter* Player)
{
	if (WaveManager)
	{
		WaveManager->StopWaves();
		WaveManager->ClearSpawnedEnemies();
	}

	DestroyRunActors();

	if (AOBGameState* OneBulletState = GetGameState<AOBGameState>())
	{
		OneBulletState->ResetRunState();
	}

	if (!Player)
	{
		Player = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	if (Player)
	{
		FVector SpawnLocation = Player->GetActorLocation();
		FRotator SpawnRotation = Player->GetActorRotation();
		FindRestartTransform(SpawnLocation, SpawnRotation);
		Player->ResetForNewRun(SpawnLocation, SpawnRotation);
	}

	if (WaveManager)
	{
		WaveManager->StartWaves();
	}
	else
	{
		InitializeWaveManager();
		if (WaveManager)
		{
			WaveManager->StartWaves();
		}
	}
}

void AOBGameMode::DestroyRunActors()
{
	for (TActorIterator<AOBBulletPickup> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}
}

bool AOBGameMode::FindRestartTransform(FVector& OutLocation, FRotator& OutRotation) const
{
	AActor* PlayerStart = UGameplayStatics::GetActorOfClass(this, APlayerStart::StaticClass());
	if (!PlayerStart)
	{
		return false;
	}

	OutLocation = PlayerStart->GetActorLocation();
	OutRotation = PlayerStart->GetActorRotation();
	return true;
}
