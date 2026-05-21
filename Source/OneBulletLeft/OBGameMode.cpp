#include "OBGameMode.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "OBBulletPickup.h"
#include "OBCharacter.h"
#include "OBGameState.h"
#include "OBHUD.h"

AOBGameMode::AOBGameMode()
{
	DefaultPawnClass = AOBCharacter::StaticClass();
	GameStateClass = AOBGameState::StaticClass();
	HUDClass = AOBHUD::StaticClass();
	EnemyClass = AOBEnemy::StaticClass();
	FastEnemyClass = AOBEnemy::StaticClass();
	HeavyEnemyClass = AOBEnemy::StaticClass();
	BulletPickupClass = AOBBulletPickup::StaticClass();

	WaveDefinitions = {
		FOBWaveDefinition{2, 0, 1.0f, 0.8f, 2},
		FOBWaveDefinition{1, 1, 6.0f, 1.0f, 3},
		FOBWaveDefinition{3, 2, 8.0f, 0.9f, 5}
	};
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

	SpawnPoints = {
		FVector(1200.0f, 1200.0f, 120.0f),
		FVector(-1200.0f, 1200.0f, 120.0f),
		FVector(1200.0f, -1200.0f, 120.0f),
		FVector(-1200.0f, -1200.0f, 120.0f),
		FVector(0.0f, 1450.0f, 120.0f),
		FVector(0.0f, -1450.0f, 120.0f)
	};

	if (bBuildGreyboxArena)
	{
		BuildGreyboxArena();
	}

	if (bUseScriptedWaves && WaveDefinitions.Num() > 0)
	{
		RestartSpawning();
	}
	else
	{
		SpawnEnemyWaveTick();
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AOBGameMode::SpawnEnemyWaveTick, SpawnInterval, true);
	}
}

void AOBGameMode::BuildGreyboxArena()
{
	SpawnBlock(FVector(0.0f, 0.0f, -60.0f), FVector(32.0f, 32.0f, 0.6f), TEXT("OB_Floor"));
	SpawnBlock(FVector(0.0f, 1600.0f, 150.0f), FVector(32.0f, 0.6f, 3.0f), TEXT("OB_Wall_N"));
	SpawnBlock(FVector(0.0f, -1600.0f, 150.0f), FVector(32.0f, 0.6f, 3.0f), TEXT("OB_Wall_S"));
	SpawnBlock(FVector(1600.0f, 0.0f, 150.0f), FVector(0.6f, 32.0f, 3.0f), TEXT("OB_Wall_E"));
	SpawnBlock(FVector(-1600.0f, 0.0f, 150.0f), FVector(0.6f, 32.0f, 3.0f), TEXT("OB_Wall_W"));

	SpawnBlock(FVector(520.0f, 420.0f, 160.0f), FVector(1.2f, 1.2f, 3.2f), TEXT("OB_Column_A"));
	SpawnBlock(FVector(-520.0f, 420.0f, 160.0f), FVector(1.2f, 1.2f, 3.2f), TEXT("OB_Column_B"));
	SpawnBlock(FVector(520.0f, -420.0f, 160.0f), FVector(1.2f, 1.2f, 3.2f), TEXT("OB_Column_C"));
	SpawnBlock(FVector(-520.0f, -420.0f, 160.0f), FVector(1.2f, 1.2f, 3.2f), TEXT("OB_Column_D"));

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		NavSystem->Build();
	}
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

void AOBGameMode::SpawnEnemyWaveTick()
{
	const int32 LiveLimit = (bUseScriptedWaves && WaveDefinitions.IsValidIndex(CurrentWaveIndex))
		? WaveDefinitions[CurrentWaveIndex].MaxLiveEnemies
		: MaxLiveEnemies;

	if (CountLiveEnemies() >= LiveLimit || SpawnPoints.Num() == 0)
	{
		return;
	}

	if (bUseScriptedWaves && WaveDefinitions.IsValidIndex(CurrentWaveIndex))
	{
		if (RemainingFastInWave <= 0 && RemainingHeavyInWave <= 0)
		{
			GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
			StartNextWave();
			return;
		}

		const bool bPreferHeavy = RemainingHeavyInWave > 0 && (RemainingFastInWave <= 0 || FMath::FRand() < 0.4f);
		if (bPreferHeavy)
		{
			if (SpawnEnemyOfType(EOBEnemyType::Heavy))
			{
				--RemainingHeavyInWave;
			}
		}
		else if (RemainingFastInWave > 0 && SpawnEnemyOfType(EOBEnemyType::Fast))
		{
			--RemainingFastInWave;
		}
		return;
	}

	const bool bSpawnHeavy = FMath::FRand() < 0.35f;
	SpawnEnemyOfType(bSpawnHeavy ? EOBEnemyType::Heavy : EOBEnemyType::Fast);
}

void AOBGameMode::StartNextWave()
{
	const int32 NextWaveIndex = CurrentWaveIndex + 1;
	if (!WaveDefinitions.IsValidIndex(NextWaveIndex))
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AOBGameMode::SpawnEnemyWaveTick, SpawnInterval, true);
		return;
	}

	const float Delay = FMath::Max(WaveDefinitions[NextWaveIndex].DelayBeforeWave, 0.0f);
	CurrentWaveIndex = NextWaveIndex;
	GetWorldTimerManager().SetTimer(WaveStartTimerHandle, FTimerDelegate::CreateUObject(this, &AOBGameMode::StartWave, CurrentWaveIndex), Delay, false);
}

void AOBGameMode::StartWave(int32 WaveIndex)
{
	if (!WaveDefinitions.IsValidIndex(WaveIndex))
	{
		return;
	}

	RemainingFastInWave = FMath::Max(WaveDefinitions[WaveIndex].FastCount, 0);
	RemainingHeavyInWave = FMath::Max(WaveDefinitions[WaveIndex].HeavyCount, 0);
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AOBGameMode::SpawnEnemyWaveTick, WaveDefinitions[WaveIndex].SpawnInterval, true, 0.0f);
}

bool AOBGameMode::SpawnEnemyOfType(EOBEnemyType Type)
{
	FVector SpawnLocation = FVector::ZeroVector;
	if (!TryChooseSpawnLocation(SpawnLocation))
	{
		return false;
	}

	TSubclassOf<AOBEnemy> ClassToSpawn = Type == EOBEnemyType::Heavy ? HeavyEnemyClass : FastEnemyClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = EnemyClass;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AOBEnemy* Enemy = GetWorld()->SpawnActor<AOBEnemy>(ClassToSpawn, SpawnLocation, FRotator::ZeroRotator, Params);
	if (Enemy)
	{
		Enemy->Configure(Type);
		return true;
	}

	return false;
}

bool AOBGameMode::TryChooseSpawnLocation(FVector& OutLocation) const
{
	if (SpawnPoints.Num() == 0)
	{
		return false;
	}

	const AOBCharacter* Player = Cast<AOBCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!bSpawnEnemiesOnlyInFrontOfPlayer || !Player)
	{
		OutLocation = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
		return true;
	}

	TArray<FVector> FrontSpawnPoints;
	const FVector PlayerLocation = Player->GetActorLocation();
	FVector PlayerForward = Player->GetActorForwardVector().GetSafeNormal2D();
	if (const AController* PlayerController = Player->GetController())
	{
		PlayerForward = FRotationMatrix(FRotator(0.0f, PlayerController->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X).GetSafeNormal2D();
	}

	for (const FVector& SpawnPoint : SpawnPoints)
	{
		const FVector ToSpawn = (SpawnPoint - PlayerLocation).GetSafeNormal2D();
		const float Dot = FVector::DotProduct(PlayerForward, ToSpawn);
		if (Dot >= FrontSpawnMinDot)
		{
			FrontSpawnPoints.Add(SpawnPoint);
		}
	}

	if (FrontSpawnPoints.Num() > 0)
	{
		OutLocation = FrontSpawnPoints[FMath::RandRange(0, FrontSpawnPoints.Num() - 1)];
		return true;
	}

	if (bAllowAnySpawnIfNoFrontPoint)
	{
		OutLocation = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
		return true;
	}

	return false;
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

void AOBGameMode::RestartRun(AOBCharacter* Player)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveStartTimerHandle);

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

	RestartSpawning();
}

int32 AOBGameMode::CountLiveEnemies() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AOBEnemy::StaticClass(), Enemies);
	return Enemies.Num();
}

void AOBGameMode::RestartSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveStartTimerHandle);

	CurrentWaveIndex = INDEX_NONE;
	RemainingFastInWave = 0;
	RemainingHeavyInWave = 0;

	if (bUseScriptedWaves && WaveDefinitions.Num() > 0)
	{
		StartNextWave();
	}
	else
	{
		SpawnEnemyWaveTick();
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AOBGameMode::SpawnEnemyWaveTick, SpawnInterval, true);
	}
}

void AOBGameMode::DestroyRunActors()
{
	for (TActorIterator<AOBEnemy> It(GetWorld()); It; ++It)
	{
		It->Destroy();
	}

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

void AOBGameMode::SpawnBlock(const FVector& Location, const FVector& Scale, const FName& Name)
{
	static UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Name = Name;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Block)
	{
		Block->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		Block->SetActorScale3D(Scale);
		Block->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
		Block->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
	}
}
