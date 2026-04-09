#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWMenuPanelWidget.generated.h"

class UTWMenuButtonWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMenuActionRequested, FName, ActionId);

UCLASS()
class CH4_PROJECT_API UTWMenuPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetMenuData(const TArray<FMenuButtonViewModel>& InButtons);

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnMenuActionRequested OnMenuActionRequested;

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTWMenuButtonWidget> ButtonSlot0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTWMenuButtonWidget> ButtonSlot1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTWMenuButtonWidget> ButtonSlot2;

private:
	void ApplySlotData(int32 SlotIndex, const FMenuButtonViewModel& InData);

	UFUNCTION()
	void HandleMenuButtonClicked(FName ActionId);
};