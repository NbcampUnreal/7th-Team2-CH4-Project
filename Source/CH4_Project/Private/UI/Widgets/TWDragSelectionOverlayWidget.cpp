#include "UI/Widgets/TWDragSelectionOverlayWidget.h"

#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"

namespace
{
	constexpr float DragLineThickness = 2.0f;
}

void UTWDragSelectionOverlayWidget::SetDragState(const FUIDragSelectionStateViewModel& InData)
{
	CachedDragState = InData;

	const bool bShouldShow =
		InData.bVisible &&
		!InData.StartScreenPosition.Equals(InData.EndScreenPosition, 0.5f);

	const ESlateVisibility NewVisibility =
		bShouldShow
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed;

	SetVisibility(NewVisibility);

	if (Border_Top)
	{
		Border_Top->SetVisibility(NewVisibility);
	}

	if (Border_Bottom)
	{
		Border_Bottom->SetVisibility(NewVisibility);
	}

	if (Border_Left)
	{
		Border_Left->SetVisibility(NewVisibility);
	}

	if (Border_Right)
	{
		Border_Right->SetVisibility(NewVisibility);
	}

	if (!bShouldShow)
	{
		return;
	}

	UpdateRect();
}

void UTWDragSelectionOverlayWidget::UpdateRect()
{
	if (!Border_Top || !Border_Bottom || !Border_Left || !Border_Right)
	{
		return;
	}

	const FVector2D MinPoint(
		FMath::Min(CachedDragState.StartScreenPosition.X, CachedDragState.EndScreenPosition.X),
		FMath::Min(CachedDragState.StartScreenPosition.Y, CachedDragState.EndScreenPosition.Y)
	);

	const FVector2D MaxPoint(
		FMath::Max(CachedDragState.StartScreenPosition.X, CachedDragState.EndScreenPosition.X),
		FMath::Max(CachedDragState.StartScreenPosition.Y, CachedDragState.EndScreenPosition.Y)
	);

	const float Width = FMath::Max(MaxPoint.X - MinPoint.X, DragLineThickness);
	const float Height = FMath::Max(MaxPoint.Y - MinPoint.Y, DragLineThickness);

	// Top
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Border_Top->Slot))
	{
		CanvasSlot->SetPosition(FVector2D(MinPoint.X, MinPoint.Y));
		CanvasSlot->SetSize(FVector2D(Width, DragLineThickness));
	}

	// Bottom
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Border_Bottom->Slot))
	{
		CanvasSlot->SetPosition(FVector2D(MinPoint.X, MaxPoint.Y - DragLineThickness));
		CanvasSlot->SetSize(FVector2D(Width, DragLineThickness));
	}

	// Left
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Border_Left->Slot))
	{
		CanvasSlot->SetPosition(FVector2D(MinPoint.X, MinPoint.Y));
		CanvasSlot->SetSize(FVector2D(DragLineThickness, Height));
	}

	// Right
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Border_Right->Slot))
	{
		CanvasSlot->SetPosition(FVector2D(MaxPoint.X - DragLineThickness, MinPoint.Y));
		CanvasSlot->SetSize(FVector2D(DragLineThickness, Height));
	}
}