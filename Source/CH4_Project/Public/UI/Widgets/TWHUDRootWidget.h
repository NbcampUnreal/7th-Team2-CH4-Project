#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWHUDRootWidget.generated.h"

class UTWTopBarWidget;
class UTWSelectionPanelWidget;
class UTWCommandPanelWidget;
class UTWNotificationPanelWidget;
class UTWMenuPanelWidget;
class UTWMinimapPanelWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHUDCommandClicked, FName);

UCLASS()
class CH4_PROJECT_API UTWHUDRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable)
	void SetTopBarData(const FTopBarViewModel& InData);

	UFUNCTION(BlueprintCallable)
	void SetSelectionData(const FSelectionViewModel& InData);

	UFUNCTION(BlueprintCallable)
	void SetCommandData(const TArray<FCommandSlotViewModel>& InCommands);

	UFUNCTION(BlueprintCallable)
	void ShowNotification(const FString& Message, ENotificationType Type);

	UFUNCTION(BlueprintCallable)
	void SetMenuData(const TArray<FMenuButtonViewModel>& InButtons);

	UFUNCTION(BlueprintCallable)
	void SetMinimapData(const FMinimapPanelViewModel& InData);

	FOnHUDCommandClicked& GetOnHUDCommandClickedDelegate()
	{
		return OnHUDCommandClicked;
	}

protected:
	void HandleCommandPanelCommandClicked(FName InCommandId);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTWTopBarWidget> TopBar = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTWSelectionPanelWidget> SelectionPanel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTWCommandPanelWidget> CommandPanel = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTWNotificationPanelWidget> NotificationPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTWMenuPanelWidget> MenuPanel = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTWMinimapPanelWidget> MinimapPanel = nullptr;

	FOnHUDCommandClicked OnHUDCommandClicked;
};