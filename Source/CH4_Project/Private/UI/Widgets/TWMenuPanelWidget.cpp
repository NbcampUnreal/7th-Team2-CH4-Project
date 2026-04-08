#include "UI/Widgets/TWMenuPanelWidget.h"
#include "UI/Widgets/TWMenuButtonWidget.h"

void UTWMenuPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ButtonSlot0)
	{
		ButtonSlot0->OnButtonClicked.AddDynamic(this, &UTWMenuPanelWidget::HandleMenuButtonClicked);
	}

	if (ButtonSlot1)
	{
		ButtonSlot1->OnButtonClicked.AddDynamic(this, &UTWMenuPanelWidget::HandleMenuButtonClicked);
	}

	if (ButtonSlot2)
	{
		ButtonSlot2->OnButtonClicked.AddDynamic(this, &UTWMenuPanelWidget::HandleMenuButtonClicked);
	}
}

void UTWMenuPanelWidget::SetMenuData(const TArray<FMenuButtonViewModel>& InButtons)
{
	for (int32 i = 0; i < 3; ++i)
	{
		FMenuButtonViewModel EmptyData;
		EmptyData.SlotIndex = i;
		EmptyData.ActionId = NAME_None;
		EmptyData.DisplayName = TEXT("");
		EmptyData.HotkeyLabel = TEXT("");
		EmptyData.bEnabled = false;
		EmptyData.bVisible = false;
		EmptyData.TooltipText = TEXT("");

		ApplySlotData(i, EmptyData);
	}

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
	case 1:
		TargetButton = ButtonSlot1;
		break;
	case 2:
		TargetButton = ButtonSlot2;
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