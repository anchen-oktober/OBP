#include "OBEnemySpawnPoint.h"

#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"

AOBEnemySpawnPoint::AOBEnemySpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
	Billboard->SetupAttachment(SceneRoot);
	Billboard->SetHiddenInGame(true);
}
