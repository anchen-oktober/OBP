#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OBEnemySpawnPoint.generated.h"

class UArrowComponent;
class UBillboardComponent;
class USceneComponent;

UCLASS(Blueprintable)
class ONEBULLETLEFT_API AOBEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AOBEnemySpawnPoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Enemy Spawn Point")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Enemy Spawn Point")
	TObjectPtr<UBillboardComponent> Billboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="OneBulletSettings|Enemy Spawn Point")
	TObjectPtr<UArrowComponent> Arrow;
};
