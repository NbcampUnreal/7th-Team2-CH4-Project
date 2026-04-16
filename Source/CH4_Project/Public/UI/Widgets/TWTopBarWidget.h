#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWTopBarWidget.generated.h"

class UTextBlock;

UCLASS()
class CH4_PROJECT_API UTWTopBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetTopBarData(const FTopBarViewModel& InData);

protected:
	FString ResolvePopulationDisplayText(const FTopBarViewModel& InData) const;
	FString FormatUpkeepText(int32 InUpkeep) const;
	FSlateColor ResolveUpkeepColor(int32 InUpkeep) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextWood = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextOre = nullptr;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextMithril = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextPopulation = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextGameTime = nullptr;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextWoodUpkeep = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextOreUpkeep = nullptr;
};
