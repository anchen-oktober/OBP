#include "OBHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "OBCharacter.h"
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

	if (bDrawMouseSensitivityPanel)
	{
		DrawMouseSensitivityPanel();
	}

	if (bDrawMouseSensitivityPanel && GetWorld() && GetWorld()->GetTimeSeconds() < MouseSensitivityDisplayEndTime)
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

void AOBHUD::ToggleMouseSensitivityPanel()
{
	SetMouseSensitivityPanelVisible(!bDrawMouseSensitivityPanel);
}

void AOBHUD::SetMouseSensitivityPanelVisible(bool bVisible)
{
	bDrawMouseSensitivityPanel = bVisible;
	if (!bDrawMouseSensitivityPanel)
	{
		MouseSensitivityDisplayEndTime = 0.0f;
	}
}

void AOBHUD::DrawMouseSensitivityPanel()
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	const AOBCharacter* PlayerCharacter = PlayerController ? Cast<AOBCharacter>(PlayerController->GetPawn()) : nullptr;
	if (!Canvas || !PlayerCharacter)
	{
		return;
	}

	const float Sensitivity = PlayerCharacter->GetMouseSensitivity();
	const float NormalizedSensitivity = PlayerCharacter->GetMouseSensitivityNormalized();
	const FString Label = FString::Printf(TEXT("Mouse sensitivity  [ / ]  %.2fx"), Sensitivity);
	constexpr float Padding = 10.0f;
	constexpr float TextScale = 0.9f;
	constexpr float PanelWidth = 290.0f;
	constexpr float PanelHeight = 50.0f;
	constexpr float BarHeight = 5.0f;
	const float X = 24.0f;
	const float Y = FMath::Max(20.0f, Canvas->ClipY - PanelHeight - 24.0f);
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;

	DrawRect(MouseSensitivityPanelColor.ToFColor(true), X, Y, PanelWidth, PanelHeight);
	DrawText(Label, MouseSensitivityTextColor.ToFColor(true), X + Padding, Y + 8.0f, Font, TextScale, false);

	const float BarX = X + Padding;
	const float BarY = Y + PanelHeight - Padding - BarHeight;
	const float BarWidth = PanelWidth - Padding * 2.0f;
	DrawRect(FLinearColor(0.20f, 0.22f, 0.24f, 0.85f).ToFColor(true), BarX, BarY, BarWidth, BarHeight);
	DrawRect(MouseSensitivityTextColor.ToFColor(true), BarX, BarY, BarWidth * NormalizedSensitivity, BarHeight);
}
