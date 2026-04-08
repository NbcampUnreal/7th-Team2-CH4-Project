#include "UI/Widgets/TWSelectionPanelWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "UI/Data/TWSelectionSummaryItemObject.h"

void UTWSelectionPanelWidget::SetSelectionData(const FSelectionViewModel& InData)
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[SelectionPanel] SetSelectionData - Name: %s / Type: %s / HP: %s / ViewMode: %d"),
		*InData.DisplayName,
		*InData.TypeLabel,
		*InData.HPText,
		static_cast<int32>(InData.ViewMode));

	if (!StateSwitcher)
	{
		return;
	}

	if (InData.SelectionType == ESelectionViewType::None || InData.ViewMode == ESelectionViewMode::None)
	{
		if (EmptyStateBox)
		{
			StateSwitcher->SetActiveWidget(EmptyStateBox);
		}

		RebuildMultiSummaryTiles({});
		return;
	}

	if (InData.ViewMode == ESelectionViewMode::Multi && MultiStateBox)
	{
		StateSwitcher->SetActiveWidget(MultiStateBox);
		RefreshMultiState(InData);
		return;
	}

	if (ContentBox)
	{
		StateSwitcher->SetActiveWidget(ContentBox);
	}

	RefreshSingleState(InData);
}

void UTWSelectionPanelWidget::RefreshSingleState(const FSelectionViewModel& InData)
{
	if (TextName)
	{
		TextName->SetText(FText::FromString(InData.DisplayName));
	}

	if (TextType)
	{
		TextType->SetText(FText::FromString(InData.TypeLabel));
	}

	if (TextHP)
	{
		TextHP->SetText(FText::FromString(InData.HPText));
	}

	if (TextCountLabel)
	{
		TextCountLabel->SetText(FText::GetEmpty());
	}

	if (ImagePortrait)
	{
		const TSoftObjectPtr<UTexture2D> PortraitToUse =
			!InData.PortraitIcon.IsNull() ? InData.PortraitIcon : InData.Portrait;

		UTexture2D* LoadedTexture = nullptr;

		if (!PortraitToUse.IsNull())
		{
			LoadedTexture = PortraitToUse.LoadSynchronous();
		}

		if (LoadedTexture)
		{
			ImagePortrait->SetBrushFromTexture(LoadedTexture, true);
			ImagePortrait->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ImagePortrait->SetBrushFromTexture(nullptr, true);
			ImagePortrait->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	RebuildMultiSummaryTiles({});
}

void UTWSelectionPanelWidget::RefreshMultiState(const FSelectionViewModel& InData)
{
	if (TextCountLabel)
	{
		TextCountLabel->SetText(FText::FromString(InData.CountLabel));
	}

	if (ImagePortrait)
	{
		ImagePortrait->SetBrushFromTexture(nullptr, true);
		ImagePortrait->SetVisibility(ESlateVisibility::Hidden);
	}

	RebuildMultiSummaryTiles(InData.SummaryItems);
}

void UTWSelectionPanelWidget::RebuildMultiSummaryTiles(const TArray<FSelectionSummaryItemViewModel>& InItems)
{
	if (!SummaryTileView)
	{
		return;
	}

	SummaryTileView->ClearListItems();

	for (const FSelectionSummaryItemViewModel& Item : InItems)
	{
		UTWSelectionSummaryItemObject* ItemObject = NewObject<UTWSelectionSummaryItemObject>(this);
		if (!ItemObject)
		{
			continue;
		}

		ItemObject->SummaryData = Item;
		SummaryTileView->AddItem(ItemObject);
	}
}