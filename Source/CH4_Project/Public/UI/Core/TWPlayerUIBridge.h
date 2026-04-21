#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UObject/Object.h"
#include "UI/Data/TWUIInputStateTypes.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWPlayerUIBridge.generated.h"

class ATWBaseBuilding;
class ATWPlayerController;
class ATWPopulationBuilding;
class ATWTroopSpawnBuilding;
class ATWUpgradeBuilding;
class UDataTable;
class UTWHUDRootWidget;
class UTWUIHUDCoordinator;
class UTWUISelectionProvider;
class UTWUIResourceProvider;
struct FKey;
struct FUICommandMetaRow;
struct FSelectionSummaryItemViewModel;
struct FSelectionViewModel;

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
	void RefreshTopBarOnly();
	int32 GetCurrentElapsedSeconds() const;

	// 기존 외부 호환용. 현재 구조에서는 실질적으로 사용하지 않음.
	bool HandleContextCommand(FName CommandId);

	const FUICommandMetaRow* FindCommandMetaRow(FName CommandId) const;
	bool TryResolveCommandIdFromHotkey(const FKey& InKey, FName& OutCommandId) const;
	bool TryGetVisibleCommandIdAtIndex(int32 Index, FName& OutCommandId) const;
	
	// 기존 외부 호환용 no-op
	void ResetContext();
	void PushContext(FName InContextId);
	bool PopContext();

	void ForceRefreshSelectionFromGameplayEvent();
	void SetArmedCommandState(FName InCommandId);
	void ClearArmedCommandState();
	void SetDragSelectionState(bool bInVisible, const FVector2D& InStart, const FVector2D& InEnd);
	void SetCursorScreenPosition(const FVector2D& InScreenPosition);
	void SetCursorOverlayVisible(bool bInVisible);
	void RefreshBuildModeNotification();
	FName GetArmedCommandId() const { return ArmedCommandId; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnUICommandRequested, FName);
	FOnUICommandRequested& GetOnUICommandRequestedDelegate() { return OnUICommandRequested; }

	void SetEdgeScrollingActive(bool bInActive);
	UTWHUDRootWidget* GetHUDRootWidget() const { return HUDRootWidget; }

protected:
	void HandleHUDCommandRequested(FName CommandId);

	// Selection refresh timer (생산/대기열/명령 직후 갱신 통합)
	void StartSelectionRefreshTimer(float InGraceSeconds = 0.0f);
	void StopSelectionRefreshTimer();
	void HandleSelectionRefreshTick();
	void UpdateSelectionRefreshPolicyFromSelectedBuilding();
	bool ShouldKeepSelectionRefreshTimerRunning() const;

	FString NormalizeHotkeyLabelFromKey(const FKey& InKey) const;
	int32 ResolveBuildingQueueCount(FName CommandId) const;

	void RefreshInputState();
	void RefreshDragSelectionState();

	// Selection refresh 리팩토링용 helper
	bool TryBuildModeSelectionVM(FSelectionViewModel& OutVM, TArray<FName>& OutCommandIds);
	bool TryBuildingSelectionVM(FSelectionViewModel& OutVM, TArray<FName>& OutCommandIds);
	bool TryUnitSelectionVM(FSelectionViewModel& OutVM, TArray<FName>& OutCommandIds);

	void FillCommonBuildingSelectionVM(ATWBaseBuilding* SelectedBuilding, FSelectionViewModel& OutVM) const;
	void FillTroopBuildingSelectionData(
		ATWTroopSpawnBuilding* TroopBuilding,
		FSelectionViewModel& InOutVM,
		TArray<FName>& OutCommandIds
	);
	void FillUpgradeBuildingSelectionData(
		ATWUpgradeBuilding* UpgradeBuilding,
		FSelectionViewModel& InOutVM,
		TArray<FName>& OutCommandIds
	);
	void FillPopulationBuildingSelectionData(
		ATWPopulationBuilding* PopulationBuilding,
		FSelectionViewModel& InOutVM,
		TArray<FName>& OutCommandIds
	);

	void ApplySelectionResult(
		const FSelectionViewModel& VM,
		const TArray<FName>& CommandIds,
		ESelectionViewMode ViewMode,
		int32 TotalCount,
		const TArray<FSelectionSummaryItemViewModel>& SummaryItems
	);
	void ApplyCommandQueueCounts(const TArray<FName>& CommandIds);
	void ClearSelectionResult();

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

	FTimerHandle SelectionRefreshTimerHandle;
	bool bSelectionRefreshTimerActive = false;
	float SelectionRefreshGraceRemaining = 0.0f;

	FOnUICommandRequested OnUICommandRequested;

	UPROPERTY()
	FName ArmedCommandId = NAME_None;

	UPROPERTY()
	FVector2D CursorScreenPosition = FVector2D::ZeroVector;

	UPROPERTY()
	FUIDragSelectionStateViewModel CachedDragSelectionState;

	UPROPERTY()
	bool bEdgeScrollingActive = false;
};