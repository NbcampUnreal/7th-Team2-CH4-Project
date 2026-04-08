#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWMenuPanelWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;

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
	TObjectPtr<UButton> ButtonSlot0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonSlot1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ButtonSlot2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextSlot0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextSlot1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextSlot2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DisabledOverlay0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DisabledOverlay1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DisabledOverlay2;

private:
	void ApplySlotData(int32 SlotIndex, const FMenuButtonViewModel& InData);
	void BroadcastAction(int32 SlotIndex);

	UFUNCTION()
	void HandleClickedSlot0();

	UFUNCTION()
	void HandleClickedSlot1();

	UFUNCTION()
	void HandleClickedSlot2();

private:
	FMenuButtonViewModel CachedSlotData[3];
};