#include "UI/Widgets/TWMinimapPanelWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UTWMinimapPanelWidget::SetMinimapData(const FMinimapPanelViewModel& InData)
{
	SetVisibility(InData.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(InData.bInteractive ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (MinimapFrame)
	{
		MinimapFrame->SetVisibility(InData.bShowFrame ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		MinimapFrame->SetBrushColor(InData.bInteractive
			? FLinearColor(0.08f, 0.08f, 0.08f, 0.9f)
			: FLinearColor(0.05f, 0.05f, 0.05f, 0.85f));
	}

	if (MinimapSurface)
	{
		MinimapSurface->SetBrushColor(InData.bInteractive
			? FLinearColor(0.12f, 0.14f, 0.12f, 0.95f)
			: FLinearColor(0.08f, 0.08f, 0.08f, 0.9f));
	}
}