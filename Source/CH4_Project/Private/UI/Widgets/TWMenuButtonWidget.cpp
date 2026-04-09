#include "UI/Widgets/TWMenuButtonWidget.h"
#include "CommonTextBlock.h"
#include "Components/Widget.h"

void UTWMenuButtonWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshFromData();
}

void UTWMenuButtonWidget::SetButtonData(const FMenuButtonViewModel& InData)
{
	CachedData = InData;
	RefreshFromData();
}

void UTWMenuButtonWidget::RefreshFromData()
{
	SetVisibility(CachedData.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetIsEnabled(CachedData.bEnabled);
	SetToolTipText(FText::FromString(CachedData.TooltipText));

	if (LabelText)
	{
		LabelText->SetText(FText::FromString(CachedData.DisplayName));
	}

	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(
			CachedData.bEnabled ? ESlateVisibility::Collapsed : ESlateVisibility::Visible
		);
	}
}

void UTWMenuButtonWidget::NativeOnClicked()
{
	Super::NativeOnClicked();

	if (!CachedData.bVisible || !CachedData.bEnabled || CachedData.ActionId.IsNone())
	{
		return;
	}

	OnButtonClicked.Broadcast(CachedData.ActionId);
}