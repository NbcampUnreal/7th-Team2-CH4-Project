#include "UI/Widgets/TWCommandSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UTWCommandSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveAll(this);
		SlotButton->OnClicked.AddDynamic(this, &UTWCommandSlotWidget::HandleButtonClicked);
	}

	RefreshVisual();
}

void UTWCommandSlotWidget::SetCommandData(const FCommandSlotViewModel& InViewModel)
{
	CachedViewModel = InViewModel;
	RefreshVisual();
}

void UTWCommandSlotWidget::HandleButtonClicked()
{
	if (CachedViewModel.CommandId.IsNone())
	{
		return;
	}

	if (!CachedViewModel.bVisible || !CachedViewModel.bEnabled)
	{
		return;
	}

	OnCommandSlotClicked.Broadcast(CachedViewModel.CommandId);
}

void UTWCommandSlotWidget::RefreshVisual()
{
	SetVisibility(CachedViewModel.bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (TextDisplayName)
	{
		TextDisplayName->SetText(FText::FromString(CachedViewModel.DisplayName));
	}

	if (TextHotkey)
	{
		TextHotkey->SetText(FText::FromString(CachedViewModel.HotkeyLabel));
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(CachedViewModel.bEnabled);
	}
	
	if (IconImage)
	{
		UTexture2D* LoadedTexture = CachedViewModel.Icon.Get();

		if (!LoadedTexture && !CachedViewModel.Icon.IsNull())
		{
			LoadedTexture = CachedViewModel.Icon.LoadSynchronous();
		}

		if (LoadedTexture)
		{
			IconImage->SetBrushFromTexture(LoadedTexture);
			IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}