#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWCommandSlotWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UBorder;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRTSCommandSlotClicked, FName);

UCLASS()
class CH4_PROJECT_API UTWCommandSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void SetCommandData(const FCommandSlotViewModel& InViewModel);

	const FCommandSlotViewModel& GetCommandData() const
	{
		return CachedViewModel;
	}

	FOnRTSCommandSlotClicked& GetOnCommandSlotClickedDelegate()
	{
		return OnCommandSlotClicked;
	}

protected:
	UFUNCTION()
	void HandleButtonClicked();

	void RefreshVisual();

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextDisplayName = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextHotkey = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage = nullptr;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ArmedBorder = nullptr;

protected:
	UPROPERTY()
	FCommandSlotViewModel CachedViewModel;

	FOnRTSCommandSlotClicked OnCommandSlotClicked;
};