#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWCommandPanelWidget.generated.h"

class UPanelWidget;
class UUniformGridPanel;
class UTWCommandSlotWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRTSCommandPanelCommandClicked, FName);

UCLASS()
class CH4_PROJECT_API UTWCommandPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	void SetCommandData(const TArray<FCommandSlotViewModel>& InCommandViewModels);
	FOnRTSCommandPanelCommandClicked& GetOnCommandClickedDelegate() { return OnCommandClicked; }

protected:
	void RebuildSlots();
	void BindSlotWidget(UTWCommandSlotWidget* InSlotWidget);
	void HandleSlotCommandClicked(FName InCommandId);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> SlotContainer = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTS UI")
	TSubclassOf<UTWCommandSlotWidget> CommandSlotWidgetClass;

	UPROPERTY()
	TArray<FCommandSlotViewModel> CachedCommandViewModels;

	UPROPERTY()
	TArray<TObjectPtr<UTWCommandSlotWidget>> SpawnedSlotWidgets;

	FOnRTSCommandPanelCommandClicked OnCommandClicked;
};