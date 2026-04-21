#include "UI/Widgets/TWCommandSlotWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

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

	if (ArmedBorder)
	{
		ArmedBorder->SetVisibility(
			InViewModel.bArmed
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed
		);
	}

	RefreshVisual();
}

void UTWCommandSlotWidget::HandleButtonClicked()
{
	if (CachedViewModel.CommandId.IsNone())
	{
		return;
	}

	if (!CachedViewModel.bEnabled)
	{
		return;
	}

	OnCommandSlotClicked.Broadcast(CachedViewModel.CommandId);
}

void UTWCommandSlotWidget::RefreshVisual()
{
	// 3x3 틀 고정을 위해 슬롯 위젯 자체는 항상 자리를 차지하게 둔다.
	// bVisible=false 인 특수 케이스만 Hidden 처리
	if (!CachedViewModel.bVisible && !CachedViewModel.CommandId.IsNone())
	{
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::Visible);

	const bool bIsEmptySlot = CachedViewModel.CommandId.IsNone();
	const bool bIsEnabledSlot = !bIsEmptySlot && CachedViewModel.bEnabled;

	if (TextDisplayName)
	{
		TextDisplayName->SetText(
			bIsEmptySlot
				? FText::GetEmpty()
				: FText::FromString(CachedViewModel.DisplayName)
		);
	}

	if (TextHotkey)
	{
		TextHotkey->SetText(
			bIsEmptySlot
				? FText::GetEmpty()
				: FText::FromString(CachedViewModel.HotkeyLabel)
		);
	}
	if (TextDescription)
	{
		const FString DescriptionText =
			!CachedViewModel.Description.IsEmpty()
				? CachedViewModel.Description
				: CachedViewModel.DisableReason;

		if (bIsEmptySlot || DescriptionText.IsEmpty())
		{
			TextDescription->SetText(FText::GetEmpty());
			TextDescription->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TextDescription->SetText(FText::FromString(DescriptionText));
			TextDescription->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(bIsEnabledSlot);
	}

	if (IconImage)
	{
		if (bIsEmptySlot)
		{
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
		else
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
				IconImage->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}