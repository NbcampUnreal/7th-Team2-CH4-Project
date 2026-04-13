#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Core/TWSelectionDataProvider.h"
#include "TWUISelectionProvider.generated.h"

UCLASS(BlueprintType)
class CH4_PROJECT_API UTWUISelectionProvider
	: public UObject
	, public ITWSelectionDataProvider
{
	GENERATED_BODY()

public:
	UTWUISelectionProvider();

	void Initialize(UObject* InSelectionSystemSource);
	void Shutdown();

	void SetUseDebugFallback(bool bInUseDebugFallback);

	UFUNCTION(BlueprintCallable, Category="UI")
	void DebugSetSelection(
		const FSelectionViewModel& InSelectionViewModel,
		const TArray<FName>& InCommandIds,
		ESelectionViewMode InViewMode,
		FName InPrimaryEntityId,
		int32 InTotalSelectedCount,
		const TArray<FSelectionSummaryItemViewModel>& InSummaryItems
	);

	UFUNCTION(BlueprintCallable, Category="UI")
	void DebugClearSelection();

	UFUNCTION(BlueprintCallable, Category="UI")
	void SetRuntimeSelection(
		const FSelectionViewModel& InSelectionViewModel,
		const TArray<FName>& InCommandIds,
		ESelectionViewMode InViewMode,
		FName InPrimaryEntityId,
		int32 InTotalSelectedCount,
		const TArray<FSelectionSummaryItemViewModel>& InSummaryItems
	);

	UFUNCTION(BlueprintCallable, Category="UI")
	void ClearRuntimeSelection();

	UFUNCTION(BlueprintCallable, Category="UI")
	void ClearRuntimeCommandData();

	UFUNCTION(BlueprintCallable, Category="UI")
	void SetRuntimeCommandQueueCount(FName CommandId, int32 QueueCount);

	UFUNCTION(BlueprintCallable, Category="UI")
	int32 GetRuntimeCommandQueueCount(FName CommandId) const;

	virtual FSelectionViewModel GetSelectionViewModel_Implementation() const override;
	virtual TArray<FName> GetCurrentCommandIds_Implementation() const override;

	virtual ESelectionViewMode GetSelectionViewMode() const override;
	virtual FName GetPrimaryEntityId() const override;
	virtual int32 GetTotalSelectedCount() const override;
	virtual TArray<FSelectionSummaryItemViewModel> GetSelectionSummary() const override;
	virtual TArray<FName> GetCommandIdsForSelectionEntity(const FName& InEntityId) const override;

	virtual FName GetCurrentCommandContext() const override;
	virtual TArray<FName> GetCommandIdsForCurrentContext() const override;
	virtual void HandleCommandInput(const FName& InCommandId, const FUICommandMetaRow* InCommandMetaRow = nullptr) override;
	virtual FOnUICommandContextChanged& GetOnCommandContextChangedDelegate() override;
	virtual FOnUISelectionChanged& GetOnSelectionChangedDelegate() override;

protected:
	void SeedDefaultCommandMap();
	void SeedDefaultContextCommandMap();

	void ResetCommandContext();
	void PushAndSetCommandContext(FName InNewContext);
	void PopCommandContext();

	void RefreshFromSourceIfNeeded() const;
	void ApplySelectionState(
		const FSelectionViewModel& InSelectionViewModel,
		const TArray<FName>& InCommandIds,
		ESelectionViewMode InViewMode,
		FName InPrimaryEntityId,
		int32 InTotalSelectedCount,
		const TArray<FSelectionSummaryItemViewModel>& InSummaryItems,
		bool bBroadcastChanged
	);

protected:
	UPROPERTY()
	TObjectPtr<UObject> SelectionSystemSource = nullptr;

	UPROPERTY()
	bool bUseDebugFallback = false;

	UPROPERTY()
	FSelectionViewModel CachedSelectionViewModel;

	UPROPERTY()
	TArray<FName> CachedCommandIds;

	UPROPERTY()
	ESelectionViewMode CachedViewMode = ESelectionViewMode::None;

	UPROPERTY()
	FName CachedPrimaryEntityId = NAME_None;

	UPROPERTY()
	int32 CachedTotalSelectedCount = 0;

	UPROPERTY()
	TArray<FSelectionSummaryItemViewModel> CachedSummaryItems;

	UPROPERTY()
	FName CurrentCommandContext = NAME_None;

	UPROPERTY()
	TArray<FName> CommandContextStack;

	TMap<FName, TArray<FName>> EntityCommandMap;
	TMap<FName, TArray<FName>> ContextCommandMap;
	TMap<FName, int32> RuntimeCommandQueueCounts;

	FOnUISelectionChanged OnSelectionChanged;
	FOnUICommandContextChanged OnCommandContextChanged;
};