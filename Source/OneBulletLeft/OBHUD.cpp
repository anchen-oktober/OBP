#include "OBHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
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

	if (!Canvas)
	{
		return;
	}

	if (bDrawCrosshair)
	{
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

	if (GetWorld() && GetWorld()->GetTimeSeconds() < MouseSensitivityDisplayEndTime)
	{
		const FColor TextColor = MouseSensitivityTextColor.ToFColor(true);
		const FString Message = FString::Printf(TEXT("Mouse Sensitivity: %.2fx"), DisplayedMouseSensitivity);
		constexpr float TextScale = 1.15f;
		float TextWidth = 0.0f;
		float TextHeight = 0.0f;
		UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
		GetTextSize(Message, TextWidth, TextHeight, Font, TextScale);
		DrawText(Message, TextColor, (Canvas->ClipX - TextWidth) * 0.5f, Canvas->ClipY * 0.78f, Font, TextScale, false);
	}
}

void AOBHUD::ShowMouseSensitivityChanged(float NewSensitivity)
{
	DisplayedMouseSensitivity = NewSensitivity;
	MouseSensitivityDisplayEndTime = GetWorld()
		? GetWorld()->GetTimeSeconds() + FMath::Max(MouseSensitivityDisplayDuration, 0.1f)
		: 0.0f;
}
