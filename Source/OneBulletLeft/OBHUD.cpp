#include "OBHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "OBHUDWidget.h"

void AOBHUD::BeginPlay()
{
	Super::BeginPlay();

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UOBHUDWidget>(GetOwningPlayerController(), HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
}

void AOBHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bDrawCrosshair || !Canvas)
	{
		return;
	}

	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const FColor DrawColor = CrosshairColor.ToFColor(true);
	const float Gap = FMath::Max(CrosshairGap, 0.0f);
	const float ArmLength = FMath::Max(CrosshairArmLength, 1.0f);
	const float Thickness = FMath::Max(CrosshairThickness, 1.0f);

	DrawRect(DrawColor, Center.X - Gap - ArmLength, Center.Y - Thickness * 0.5f, ArmLength, Thickness);
	DrawRect(DrawColor, Center.X + Gap, Center.Y - Thickness * 0.5f, ArmLength, Thickness);
	DrawRect(DrawColor, Center.X - Thickness * 0.5f, Center.Y - Gap - ArmLength, Thickness, ArmLength);
	DrawRect(DrawColor, Center.X - Thickness * 0.5f, Center.Y + Gap, Thickness, ArmLength);
}
