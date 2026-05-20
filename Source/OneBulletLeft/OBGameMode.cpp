#include "OBGameMode.h"

#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Controller.h"
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
}

void AOBGameMode::BeginPlay()
{
	Super::BeginPlay();

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

	SpawnEnemyWaveTick();
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AOBGameMode::SpawnEnemyWaveTick, SpawnInterval, true);
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

void AOBGameMode::SpawnEnemyWaveTick()
{
	if (CountLiveEnemies() >= MaxLiveEnemies || SpawnPoints.Num() == 0)
	{
		return;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	if (!TryChooseSpawnLocation(SpawnLocation))
	{
		return;
	}

	const bool bSpawnHeavy = FMath::FRand() < 0.35f;
	TSubclassOf<AOBEnemy> ClassToSpawn = bSpawnHeavy ? HeavyEnemyClass : FastEnemyClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = EnemyClass;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AOBEnemy* Enemy = GetWorld()->SpawnActor<AOBEnemy>(ClassToSpawn, SpawnLocation, FRotator::ZeroRotator, Params);
	if (Enemy)
	{
		Enemy->Configure(bSpawnHeavy ? EOBEnemyType::Heavy : EOBEnemyType::Fast);
	}
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

int32 AOBGameMode::CountLiveEnemies() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(this, AOBEnemy::StaticClass(), Enemies);
	return Enemies.Num();
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
