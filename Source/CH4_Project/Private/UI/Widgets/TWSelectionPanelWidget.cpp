#include "UI/Widgets/TWSelectionPanelWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "UI/Data/TWSelectionQueueItemObject.h"
#include "UI/Data/TWSelectionSummaryItemObject.h"

float UTWSelectionPanelWidget::ResolveSafeHPPercent(const FSelectionViewModel& InData) const
{
	if (InData.MaxHP <= 0.f)
	{
		return 0.f;
	}

	return FMath::Clamp(InData.CurrentHP / InData.MaxHP, 0.f, 1.f);
}

void UTWSelectionPanelWidget::ApplyHPColor(UProgressBar* InProgressBar, float InPercent) const
{
	if (!InProgressBar)
	{
		return;
	}

	if (InPercent <= 0.3f)
	{
		InProgressBar->SetFillColorAndOpacity(FLinearColor(0.9f, 0.2f, 0.2f, 1.f));
	}
	else if (InPercent <= 0.7f)
	{
		InProgressBar->SetFillColorAndOpacity(FLinearColor(0.9f, 0.8f, 0.2f, 1.f));
	}
	else
	{
		InProgressBar->SetFillColorAndOpacity(FLinearColor(0.2f, 0.85f, 0.3f, 1.f));
	}
}

FText UTWSelectionPanelWidget::FormatStatInt(const TCHAR* Label, float Value) const
{
	return FText::FromString(
		FString::Printf(TEXT("%s: %d"), Label, FMath::RoundToInt(Value))
	);
}

FText UTWSelectionPanelWidget::FormatStatFloat(const TCHAR* Label, float Value) const
{
	return FText::FromString(
		FString::Printf(TEXT("%s: %.2f"), Label, Value)
	);
}

void UTWSelectionPanelWidget::SetSelectionData(const FSelectionViewModel& InData)
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[SelectionPanel] SetSelectionData - Name: %s / Type: %s / HP: %s / ViewMode: %d / SummaryCount: %d / ProductionVisible: %d / QueueCount: %d / DetailStats: %d"),
		*InData.DisplayName,
		*InData.TypeLabel,
		*InData.HPText,
		static_cast<int32>(InData.ViewMode),
		InData.SummaryItems.Num(),
		InData.bShowProductionPanel ? 1 : 0,
		InData.Production.QueueItems.Num(),
		InData.bShowDetailStats ? 1 : 0
	);

	if (!StateSwitcher)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SelectionPanel] StateSwitcher is null"));
		return;
	}

	if (InData.SelectionType == ESelectionViewType::None || InData.ViewMode == ESelectionViewMode::None)
	{
		if (EmptyStateBox)
		{
			StateSwitcher->SetActiveWidget(EmptyStateBox);
		}

		ClearAllDynamicViews();
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

	if (HPProgressBar)
	{
		const float SafePercent = ResolveSafeHPPercent(InData);
		HPProgressBar->SetPercent(SafePercent);
		ApplyHPColor(HPProgressBar, SafePercent);
	}

	if (HPBarBox)
	{
		const bool bShowHP = InData.MaxHP > 0.f;
		HPBarBox->SetVisibility(bShowHP ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	else
	{
		if (HPProgressBar)
		{
			HPProgressBar->SetVisibility(InData.MaxHP > 0.f ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}

		if (TextHP)
		{
			TextHP->SetVisibility(InData.MaxHP > 0.f ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
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

	if (StatsBox)
	{
		StatsBox->SetVisibility(
			InData.bShowDetailStats
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed
		);
	}

	if (TextDamage)
	{
		TextDamage->SetText(FormatStatInt(TEXT("ATK"), InData.Damage));
	}

	if (TextArmor)
	{
		TextArmor->SetText(FormatStatInt(TEXT("DEF"), InData.Armor));
	}

	if (TextAttackSpeed)
	{
		TextAttackSpeed->SetText(FormatStatFloat(TEXT("AS"), InData.AttackSpeed));
	}

	if (TextMoveSpeed)
	{
		TextMoveSpeed->SetText(FormatStatFloat(TEXT("SPD"), InData.MoveSpeed));
	}

	if (TextRange)
	{
		TextRange->SetText(FormatStatInt(TEXT("RNG"), InData.Range));
	}

	RebuildMultiSummaryTiles({});

	if (ProductionPanelBox)
	{
		ProductionPanelBox->SetVisibility(
			InData.bShowProductionPanel
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed
		);
	}

	if (TextProductionTitle)
	{
		TextProductionTitle->SetText(FText::FromString(InData.Production.Title));
	}

	if (TextProductionProgress)
	{
		TextProductionProgress->SetText(FText::FromString(InData.Production.ProgressText));
	}

	if (ProductionProgressBar)
	{
		const float SafeRatio = FMath::Clamp(InData.Production.ProgressRatio, 0.f, 1.f);
		ProductionProgressBar->SetPercent(SafeRatio);
	}

	if (InData.bShowProductionPanel)
	{
		RebuildProductionQueueTiles(InData.Production.QueueItems);
	}
	else
	{
		RebuildProductionQueueTiles({});
	}
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

	if (HPBarBox)
	{
		HPBarBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		if (HPProgressBar)
		{
			HPProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (TextHP)
		{
			TextHP->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (StatsBox)
	{
		StatsBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	RebuildMultiSummaryTiles(InData.SummaryItems);

	if (ProductionPanelBox)
	{
		ProductionPanelBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TextProductionTitle)
	{
		TextProductionTitle->SetText(FText::GetEmpty());
	}

	if (TextProductionProgress)
	{
		TextProductionProgress->SetText(FText::GetEmpty());
	}

	RebuildProductionQueueTiles({});
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

void UTWSelectionPanelWidget::RebuildProductionQueueTiles(const TArray<FProductionQueueItemViewModel>& InItems)
{
	if (!ProductionQueueTileView)
	{
		return;
	}

	ProductionQueueTileView->ClearListItems();

	for (const FProductionQueueItemViewModel& Item : InItems)
	{
		UTWSelectionQueueItemObject* ItemObject = NewObject<UTWSelectionQueueItemObject>(this);
		if (!ItemObject)
		{
			continue;
		}

		ItemObject->QueueData = Item;
		ProductionQueueTileView->AddItem(ItemObject);
	}
}

void UTWSelectionPanelWidget::ClearAllDynamicViews()
{
	if (TextName)
	{
		TextName->SetText(FText::GetEmpty());
	}

	if (TextType)
	{
		TextType->SetText(FText::GetEmpty());
	}

	if (TextHP)
	{
		TextHP->SetText(FText::GetEmpty());
	}

	if (HPProgressBar)
	{
		HPProgressBar->SetPercent(0.f);
	}

	if (HPBarBox)
	{
		HPBarBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TextCountLabel)
	{
		TextCountLabel->SetText(FText::GetEmpty());
	}

	if (ImagePortrait)
	{
		ImagePortrait->SetBrushFromTexture(nullptr, true);
		ImagePortrait->SetVisibility(ESlateVisibility::Hidden);
	}

	if (StatsBox)
	{
		StatsBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TextDamage)
	{
		TextDamage->SetText(FText::GetEmpty());
	}

	if (TextArmor)
	{
		TextArmor->SetText(FText::GetEmpty());
	}

	if (TextAttackSpeed)
	{
		TextAttackSpeed->SetText(FText::GetEmpty());
	}

	if (TextMoveSpeed)
	{
		TextMoveSpeed->SetText(FText::GetEmpty());
	}

	if (TextRange)
	{
		TextRange->SetText(FText::GetEmpty());
	}

	if (ProductionPanelBox)
	{
		ProductionPanelBox->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TextProductionTitle)
	{
		TextProductionTitle->SetText(FText::GetEmpty());
	}

	if (TextProductionProgress)
	{
		TextProductionProgress->SetText(FText::GetEmpty());
	}

	if (ProductionProgressBar)
	{
		ProductionProgressBar->SetPercent(0.f);
	}

	RebuildMultiSummaryTiles({});
	RebuildProductionQueueTiles({});
}