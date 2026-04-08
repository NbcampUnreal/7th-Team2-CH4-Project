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
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextWood;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextOre;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextPopulation;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextGameTime;
};