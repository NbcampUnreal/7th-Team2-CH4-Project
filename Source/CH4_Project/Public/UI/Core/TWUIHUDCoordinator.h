#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWUIHUDCoordinator.generated.h"

class UDataTable;
class UTWHUDRootWidget;



UCLASS()
class CH4_PROJECT_API UTWUIHUDCoordinator : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(
		UTWHUDRootWidget* InHUDRoot,
		UObject* InSelectionProvider,
		UObject* InResourceProvider,
		UDataTable* InCommandMetaTable,
		UDataTable* InSelectionPresentationTable);

	void Shutdown();

	UFUNCTION(BlueprintCallable)
	void RefreshAll();

	UFUNCTION(BlueprintCallable)
	void RefreshTopBar();

	UFUNCTION(BlueprintCallable)
	void RefreshSelectionPanel();

	UFUNCTION(BlueprintCallable)
	void RefreshCommandPanel();

	UFUNCTION(BlueprintCallable)
	void RefreshMenuPanel();

	UFUNCTION(BlueprintCallable)
	void RefreshMinimapPanel();

	UFUNCTION(BlueprintCallable)
	void PushNotification(const FString& Message, ENotificationType Type);

	const TArray<FCommandSlotViewModel>& GetLastBuiltCommandViewModels() const
	{
		return LastBuiltCommandViewModels;
	}

	const FUICommandMetaRow* FindCommandMetaRow(const FName& InCommandId) const;

protected:
	void BindProviderDelegates();
	void UnbindProviderDelegates();

	void HandleSelectionChanged();
	void HandleResourceChanged();
	void HandleCommandContextChanged();

	FTopBarViewModel BuildTopBarViewModel() const;
	FSelectionViewModel BuildSelectionViewModel() const;
	TArray<FCommandSlotViewModel> BuildCommandViewModels() const;
	FCommandSlotViewModel BuildCommandSlotViewModel(const FName& CommandId, int32 FallbackSlotIndex) const;
	TArray<FName> BuildCommonCommandIds() const;
	FString FormatGameTime(int32 InElapsedSeconds) const;
	

	const FUISelectionPresentationRow* FindSelectionPresentationRow(const FName& InSelectionId) const;
	void HydrateSummaryItemFromPresentation(FSelectionSummaryItemViewModel& InOutItem) const;

	bool HasValidSelectionProvider() const;
	bool HasValidResourceProvider() const;

	class ITWSelectionDataProvider* GetSelectionProviderInterface() const;
	class ITWResourceDataProvider* GetResourceProviderInterface() const;

protected:
	UPROPERTY()
	TObjectPtr<UTWHUDRootWidget> HUDRoot = nullptr;

	UPROPERTY()
	TObjectPtr<UObject> SelectionProvider = nullptr;

	UPROPERTY()
	TObjectPtr<UObject> ResourceProvider = nullptr;

	UPROPERTY()
	TObjectPtr<UDataTable> CommandMetaTable = nullptr;

	UPROPERTY()
	TObjectPtr<UDataTable> SelectionPresentationTable = nullptr;

	UPROPERTY()
	TArray<FCommandSlotViewModel> LastBuiltCommandViewModels;
	
};