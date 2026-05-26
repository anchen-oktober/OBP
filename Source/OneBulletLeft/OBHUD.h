#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OBHUD.generated.h"

class UOBHUDWidget;

UCLASS(PrioritizeCategories = "OneBulletSettings")
class ONEBULLETLEFT_API AOBHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|HUD")
	TSubclassOf<UOBHUDWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	TObjectPtr<UOBHUDWidget> HUDWidget;
};
