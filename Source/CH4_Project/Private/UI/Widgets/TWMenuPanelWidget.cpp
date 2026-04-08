#include "UI/Widgets/TWMenuPanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UTWMenuPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ButtonSlot0)
	{
		ButtonSlot0->OnClicked.AddDynamic(this, &UTWMenuPanelWidget::HandleClickedSlot0);
	}

	if (ButtonSlot1)
	{
		ButtonSlot1->OnClicked.AddDynamic(this, &UTWMenuPanelWidget::HandleClickedSlot1);
	}

	if (ButtonSlot2)
	{
		ButtonSlot2->OnClicked.AddDynamic(this, &UTWMenuPanelWidget::HandleClickedSlot2);
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
	CachedSlotData[SlotIndex] = InData;

	UButton* TargetButton = nullptr;
	UTextBlock* TargetText = nullptr;
	UWidget* TargetOverlay = nullptr;

	switch (SlotIndex)
	{
	case 0:
		TargetButton = ButtonSlot0;
		TargetText = TextSlot0;
		TargetOverlay = DisabledOverlay0;
		break;

	case 1:
		TargetButton = ButtonSlot1;
		TargetText = TextSlot1;
		TargetOverlay = DisabledOverlay1;
		break;

	case 2:
		TargetButton = ButtonSlot2;
		TargetText = TextSlot2;
		TargetOverlay = DisabledOverlay2;
		break;

	default:
		return;
	}

	if (TargetButton)
	{
		TargetButton->SetVisibility(InData.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		TargetButton->SetIsEnabled(InData.bEnabled);
	}

	if (TargetText)
	{
		TargetText->SetText(FText::FromString(InData.DisplayName));
	}

	if (TargetOverlay)
	{
		TargetOverlay->SetVisibility(InData.bEnabled ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if (TargetButton)
	{
		TargetButton->SetToolTipText(FText::FromString(InData.TooltipText));
	}
}

void UTWMenuPanelWidget::BroadcastAction(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= 3)
	{
		return;
	}

	const FMenuButtonViewModel& Data = CachedSlotData[SlotIndex];

	if (!Data.bVisible || !Data.bEnabled || Data.ActionId.IsNone())
	{
		return;
	}

	OnMenuActionRequested.Broadcast(Data.ActionId);
}

void UTWMenuPanelWidget::HandleClickedSlot0()
{
	BroadcastAction(0);
}

void UTWMenuPanelWidget::HandleClickedSlot1()
{
	BroadcastAction(1);
}

void UTWMenuPanelWidget::HandleClickedSlot2()
{
	BroadcastAction(2);
}