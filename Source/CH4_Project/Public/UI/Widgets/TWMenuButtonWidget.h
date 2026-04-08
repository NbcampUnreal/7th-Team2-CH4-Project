#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWMenuButtonWidget.generated.h"

class UCommonTextBlock;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTWMenuButtonClicked, FName, ActionId);

UCLASS()
class CH4_PROJECT_API UTWMenuButtonWidget : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetButtonData(const FMenuButtonViewModel& InData);

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnTWMenuButtonClicked OnButtonClicked;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeOnClicked() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> LabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DisabledOverlay;

private:
	void RefreshFromData();

private:
	FMenuButtonViewModel CachedData;
};