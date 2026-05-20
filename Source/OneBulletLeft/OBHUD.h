#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OBHUD.generated.h"

class UOBHUDWidget;

UCLASS()
class ONEBULLETLEFT_API AOBHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="One Bullet|HUD")
	TSubclassOf<UOBHUDWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="One Bullet|HUD")
	TObjectPtr<UOBHUDWidget> HUDWidget;
};
