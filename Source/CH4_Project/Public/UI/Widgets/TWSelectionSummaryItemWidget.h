#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWSelectionSummaryItemWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class CH4_PROJECT_API UTWSelectionSummaryItemWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetSummaryData(const FSelectionSummaryItemViewModel& InData);

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextName = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextCount = nullptr;
};