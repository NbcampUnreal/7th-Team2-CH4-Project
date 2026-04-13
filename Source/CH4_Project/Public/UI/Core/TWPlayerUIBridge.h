#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UObject/Object.h"
#include "TWPlayerUIBridge.generated.h"

class ATWPlayerController;
class UDataTable;
class UTWHUDRootWidget;
class UTWUIHUDCoordinator;
class UTWUISelectionProvider;
class UTWUIResourceProvider;
struct FKey;
struct FUICommandMetaRow;

UCLASS()
class CH4_PROJECT_API UTWPlayerUIBridge : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(
		ATWPlayerController* InOwnerController,
		TSubclassOf<UTWHUDRootWidget> InHUDRootWidgetClass,
		UDataTable* InCommandMetaTable,
		UDataTable* InSelectionPresentationTable,
		bool bInUseDebugFallback,
		FName InDefaultSelectedUnitId,
		FName InDefaultSelectedBuildingId,
		FName InDefaultMultiSelectedUnitId
	);

	void RefreshAll();
	void RefreshSelection();
	void RefreshResources();

	bool HandleContextCommand(FName CommandId);
	const FUICommandMetaRow* FindCommandMetaRow(FName CommandId) const;
	bool TryResolveCommandIdFromHotkey(const FKey& InKey, FName& OutCommandId) const;

	void ResetContext();
	void PushContext(FName InContextId);
	bool PopContext();

	void ForceRefreshSelectionFromGameplayEvent();

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnUICommandRequested, FName);
	FOnUICommandRequested& GetOnUICommandRequestedDelegate() { return OnUICommandRequested; }

protected:
	void HandleHUDCommandRequested(FName CommandId);

	void StartProductionSelectionRefresh();
	void StopProductionSelectionRefresh();
	void HandleProductionSelectionRefreshTick();

	void StartPostCommandSelectionRefreshWindow();
	void StopPostCommandSelectionRefreshWindow();
	void HandlePostCommandSelectionRefreshTick();

	FString NormalizeHotkeyLabelFromKey(const FKey& InKey) const;
	int32 ResolveBuildingQueueCount(FName CommandId) const;

protected:
	UPROPERTY()
	TObjectPtr<ATWPlayerController> OwnerController = nullptr;

	UPROPERTY()
	TSubclassOf<UTWHUDRootWidget> HUDRootWidgetClass;

	UPROPERTY()
	TObjectPtr<UTWHUDRootWidget> HUDRootWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UTWUIHUDCoordinator> HUDCoordinator = nullptr;

	UPROPERTY()
	TObjectPtr<UTWUISelectionProvider> SelectionProvider = nullptr;

	UPROPERTY()
	TObjectPtr<UTWUIResourceProvider> ResourceProvider = nullptr;

	UPROPERTY()
	TObjectPtr<UDataTable> CommandMetaTable = nullptr;

	UPROPERTY()
	TObjectPtr<UDataTable> SelectionPresentationTable = nullptr;

	UPROPERTY()
	bool bUseDebugFallback = false;

	UPROPERTY()
	FName DefaultSelectedUnitId = NAME_None;

	UPROPERTY()
	FName DefaultSelectedBuildingId = NAME_None;

	UPROPERTY()
	FName DefaultMultiSelectedUnitId = NAME_None;

	UPROPERTY()
	TArray<FName> CurrentVisibleCommandIds;

	UPROPERTY()
	TArray<FName> ContextStack;

	UPROPERTY()
	FName CurrentContextId = NAME_None;

	FTimerHandle ProductionSelectionRefreshTimerHandle;
	bool bProductionSelectionRefreshActive = false;

	FTimerHandle PostCommandSelectionRefreshTimerHandle;
	bool bPostCommandSelectionRefreshActive = false;
	float PostCommandSelectionRefreshRemaining = 0.f;

	FOnUICommandRequested OnUICommandRequested;
};