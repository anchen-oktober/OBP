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
	virtual void DrawHUD() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="OneBulletSettings|HUD")
	TSubclassOf<UOBHUDWidget> HUDWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="OneBulletSettings|HUD")
	TObjectPtr<UOBHUDWidget> HUDWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Crosshair")
	bool bDrawCrosshair = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Crosshair", meta=(ClampMin="1.0"))
	float CrosshairGap = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Crosshair", meta=(ClampMin="1.0"))
	float CrosshairArmLength = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Crosshair", meta=(ClampMin="1.0"))
	float CrosshairThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Crosshair")
	FLinearColor CrosshairColor = FLinearColor::White;
};
