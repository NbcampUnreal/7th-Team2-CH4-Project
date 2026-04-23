#include "UI/Widgets/TWMenuPanelWidget.h"
#include "UI/Widgets/TWMenuButtonWidget.h"

void UTWMenuPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ButtonSlot0)
	{
		ButtonSlot0->OnButtonClicked.AddDynamic(this, &UTWMenuPanelWidget::HandleMenuButtonClicked);
	}
}

void UTWMenuPanelWidget::SetMenuData(const TArray<FMenuButtonViewModel>& InButtons)
{
	FMenuButtonViewModel EmptyData;
	EmptyData.SlotIndex = 0;
	EmptyData.ActionId = NAME_None;
	EmptyData.DisplayName = TEXT("");
	EmptyData.HotkeyLabel = TEXT("");
	EmptyData.bEnabled = false;
	EmptyData.bVisible = false;
	EmptyData.TooltipText = TEXT("");

	ApplySlotData(0, EmptyData);
	for (const FMenuButtonViewModel& ButtonData : InButtons)
	{
		if (ButtonData.SlotIndex >= 0 && ButtonData.SlotIndex < 3)
		{
			ApplySlotData(ButtonData.SlotIndex, ButtonData);
		}
	}
}

void UTWMenuPanelWidget::ApplySlotData(int32 SlotIndex, const FMenuButtonViewModel& InData)
{
	UTWMenuButtonWidget* TargetButton = nullptr;

	switch (SlotIndex)
	{
	case 0:
		TargetButton = ButtonSlot0;
		break;
	default:
		return;
	}

	if (TargetButton)
	{
		TargetButton->SetButtonData(InData);
	}
}

void UTWMenuPanelWidget::HandleMenuButtonClicked(FName ActionId)
{
	if (!ActionId.IsNone())
	{
		OnMenuActionRequested.Broadcast(ActionId);
	}
}