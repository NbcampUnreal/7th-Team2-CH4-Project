#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWSelectionPanelWidget.generated.h"

class UImage;
class UPanelWidget;
class UTextBlock;
class UWidget;
class UWidgetSwitcher;
class UTWSelectionSummaryItemWidget;

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
	void RebuildMultiSummaryChildren(const TArray<FSelectionSummaryItemViewModel>& InItems);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextName = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextType = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextHP = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> EmptyStateBox = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ContentBox = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> StateSwitcher = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MultiStateBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ImagePortrait = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextCountLabel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SummaryContainer = nullptr;

	// 5단계 멀티 요약용 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTS UI")
	TSubclassOf<UTWSelectionSummaryItemWidget> SummaryItemWidgetClass;
};