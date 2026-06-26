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

	UFUNCTION(BlueprintCallable, Category="OneBulletSettings|HUD|Settings")
	void ShowMouseSensitivityChanged(float NewSensitivity);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Settings", meta=(ClampMin="0.1"))
	float MouseSensitivityDisplayDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OneBulletSettings|HUD|Settings")
	FLinearColor MouseSensitivityTextColor = FLinearColor(0.86f, 0.96f, 1.0f, 1.0f);

private:
	float DisplayedMouseSensitivity = 1.0f;
	float MouseSensitivityDisplayEndTime = 0.0f;
};
