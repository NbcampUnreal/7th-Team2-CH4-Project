#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWSelectionPanelWidget.generated.h"

class UImage;
class UTextBlock;
class UTileView;
class UWidget;
class UWidgetSwitcher;

UCLASS()
class CH4_PROJECT_API UTWSelectionPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetSelectionData(const FSelectionViewModel& InData);

protected:
	void RefreshSingleState(const FSelectionViewModel& InData);
	void RefreshMultiState(const FSelectionViewModel& InData);

	void RebuildMultiSummaryTiles(const TArray<FSelectionSummaryItemViewModel>& InItems);
	void RebuildProductionQueueTiles(const TArray<FProductionQueueItemViewModel>& InItems);
	void ClearAllDynamicViews();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> StateSwitcher = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> EmptyStateBox = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ContentBox = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> MultiStateBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ImagePortrait = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextName = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextType = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextHP = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextCountLabel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTileView> SummaryTileView = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ProductionPanelBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTileView> ProductionQueueTileView = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextProductionTitle = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextProductionProgress = nullptr;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProductionProgressBar = nullptr;
};