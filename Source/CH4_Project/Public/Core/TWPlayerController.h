#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "MassEntityHandle.h"
#include "GameFramework/PlayerController.h"
#include "Data/TWUnitStatus.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWPlayerController.generated.h"

class ATWBaseBuilding;
struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UDataTable;
class UTWBuildComponent;
class UTWPlayerUIBridge;
class UTWHUDRootWidget;
class UTWSelectionVisualManager;

UCLASS()
class CH4_PROJECT_API ATWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATWPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Common = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_UnitCommand = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Build = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Debug = nullptr;
	
	bool bUnitCommandContextActive = false;
	bool bBuildContextActive = false;
	bool bBuildShortcutModeActive = false;
	
	void SetMappingContextActive(UInputMappingContext* MappingContext, int32 Priority, bool bShouldBeActive, bool& bCurrentActive);
	void RefreshDynamicMappingContexts();
	bool ShouldUseUnitCommandContext() const;
	bool ShouldUseBuildContext() const;
	
	

#pragma region Input
protected:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LeftMouseAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RightMouseAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveCommandAction; // m

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackCommandAction; // a

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HoldCommandAction; // h
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScreenEdgeEnterMargin = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScreenEdgeExitMargin = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScrollSpeed = 1500.0f;

	UFUNCTION()
	void OnStartLeftMouseAction(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnEndLeftMouseAction(const FInputActionValue& InputActionValue);

	UFUNCTION()
	void OnRightMouseAction(const FInputActionValue& InputActionValue); // 명령 있을 시 취소, 없으면 이동명령

	void OnMoveCommandAction(const FInputActionValue& InputActionValue);
	void OnAttackCommandAction(const FInputActionValue& InputActionValue);
	void OnHoldCommandAction(const FInputActionValue& InputActionValue);

	UFUNCTION(Server, Reliable)
	void ServerHandleMoveCommand(const FVector& CommandLocation);

	UFUNCTION(Server, Reliable)
	void ServerHandleAttackCommand(const FVector& CommandLocation);

	UFUNCTION(Server, Reliable)
	void ServerHandleHoldCommand();

	UFUNCTION(Server, Reliable)
	void ServerHandleSingleSelect(const FVector& CommandLocation);

	UFUNCTION(Server, Reliable)
	void ServerHandleMultipleSelect(const FVector& StartLocation, const FVector& EndLocation);

	UFUNCTION(Server, Reliable)
	void ServerHandleBuildingSelect(ATWBaseBuilding* TargetBuilding);

private:
	bool HandleScreenEdgeScrolling(float DeltaSeconds);
#pragma endregion

#pragma region 병력 스폰 대기열
protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_TestSpawnTroop;

	UFUNCTION()
	void HandleTestSpawnTroop(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestSpawnTroop();
#pragma endregion

#pragma region 인구 수 대기열
protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_TestIncreasePopulation;

	UFUNCTION()
	void HandleTestIncreasePopulation(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestIncreasePopulation();
#pragma endregion

#pragma region 넥서스 데미지
protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_TestDamageBlockingBuilding;

	UFUNCTION()
	void HandleTestDamageBlockingBuilding(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestDamageBlockingBuilding();
#pragma endregion

#pragma region 건설
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleBuildModeAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectWoodCommandAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectStoneCommandAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectPopulationCommandAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectTroopCommandAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectUpgradeCommandAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectBlockingCommandAction;

	void OnToggleBuildModeAction();
	void OnSelectWoodBuildingCommandAction();
	void OnSelectStoneBuildingCommandAction();
	void OnSelectPopulationBuildingCommandAction();
	void OnSelectTroopBuildingCommandAction();
	void OnSelectUpgradeBuildingCommandAction();
	void OnSelectBlockingBuildingCommandAction();
	
public:
	UFUNCTION(BlueprintCallable, Category = "Component")
	UTWBuildComponent* GetBuildComponent() const { return BuildComponent; }
protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTWBuildComponent> BuildComponent;
#pragma endregion
	
#pragma region 업그레이드
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_TestUpgrade;

	UFUNCTION()
	void HandleTestUpgrade(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestUpgrade();
#pragma endregion
	
#pragma region UI
protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UTWHUDRootWidget> HUDRootWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UDataTable> CommandMetaTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UDataTable> SelectionPresentationTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	bool bUseUIDebugFallback = false;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FName DefaultSelectedUnitId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FName DefaultSelectedBuildingId = NAME_None;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FName DefaultMultiSelectedUnitId = NAME_None;

	UPROPERTY(Transient)
	TObjectPtr<UTWPlayerUIBridge> PlayerUIBridge = nullptr;
	
	UPROPERTY(Transient)
	TObjectPtr<UTWSelectionVisualManager> SelectionVisualManager = nullptr;
	
	int32 LocalSelectedOwnerPlayerSlot = INDEX_NONE;

private:
	void InitializeUIBridge();
	void RefreshUIBridge();
	void InitializeSelectionVisualManager();
	void RefreshSelectionVisualManager();

	void ClearLocalSelectionCache();
	void RebuildLocalSelectionSummary(const TArray<FName>& InUnitIds);
	void BuildSelectionPayloadFromEntities(
		const TArray<FMassEntityHandle>& InSelectedEntities,
		TArray<FMassNetworkID>& OutUnitIds,
		float& OutPrimaryHealth,
		bool& bOutHasPrimaryHealth);

	const FUICommandMetaRow* FindCommandMetaRowFromTable(FName CommandId) const;

	UFUNCTION(Client, Reliable)
	void ClientApplyUnitSelection(const TArray<FMassNetworkID>& InNetworkIds, float InPrimaryHealth, bool bInHasPrimaryHealth, int32 InSelectedOwnerPlayerSlot);

	UFUNCTION(Client, Reliable)
	void ClientApplyBuildingSelection(ATWBaseBuilding* InBuilding);

	UFUNCTION(Client, Reliable)
	void ClientClearSelection();

	UFUNCTION()
	void HandleUICommandRequested(FName CommandId);

	UFUNCTION(Server, Reliable)
	void ServerHandleUICommandRequested(FName CommandId);

	bool TryInvokeBuildingProduceById(ATWBaseBuilding* TargetBuilding, FName UnitId) const;
	bool TryInvokeBuildingProduceFallback(ATWBaseBuilding* TargetBuilding) const;
#pragma endregion

public:
	int32 GetLocalSelectedUnitCount() const { return LocalSelectedUnitCount; }
	FName GetLocalPrimarySelectedUnitId() const { return LocalPrimarySelectedUnitId; }
	const TArray<FSelectionSummaryItemViewModel>& GetLocalSelectionSummaryItems() const { return LocalSelectionSummaryItems; }
	bool HasLocalPrimarySelectedUnitStatus() const { return bHasLocalPrimarySelectedUnitStatus; }
	const FTWUnitStatus& GetLocalPrimarySelectedUnitStatus() const { return LocalPrimarySelectedUnitStatus; }
	ATWBaseBuilding* GetSelectedBuilding() const { return SelectedBuilding; }
	int32 GetLocalSelectedOwnerPlayerSlot() const { return LocalSelectedOwnerPlayerSlot; }

	FName ResolveBuildingSelectionId(const ATWBaseBuilding* InBuilding) const;
	void NotifyResourceStateChanged();

	UFUNCTION(Client, Reliable)
	void ClientForceRefreshSelectionBridge();
	
private:
	void ChangeCurrentCommandType(ETWCommandType CommandType);
	
	void UpdateInputOverlayState();
	void UpdateDragSelectionOverlay();
	void UpdateCursorOverlayPosition();
	FName ConvertCommandTypeToCommandId(ETWCommandType InCommandType) const;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float DragSelectionScreenThreshold = 8.f;

	bool bIsLeftMousePressed = false;
	bool bIsDraggingSelectionVisual = false;
	bool bConsumeLeftMouseRelease = false;

	FVector2D DragStartScreenPosition = FVector2D::ZeroVector;
	FVector2D CurrentMouseScreenPosition = FVector2D::ZeroVector;

private:
	ETWCommandType CurrentCommandType = ETWCommandType::None;

	UPROPERTY(Transient)
	TArray<FMassEntityHandle> ServerSelectedEntities;
	UPROPERTY(Transient)
	TArray<FMassNetworkID> ClientSelectedEntities;


	UPROPERTY(Transient)
	TObjectPtr<ATWBaseBuilding> SelectedBuilding = nullptr;

	UPROPERTY(Transient)
	int32 LocalSelectedUnitCount = 0;

	UPROPERTY(Transient)
	FName LocalPrimarySelectedUnitId = NAME_None;

	UPROPERTY(Transient)
	FTWUnitStatus LocalPrimarySelectedUnitStatus;

	UPROPERTY(Transient)
	bool bHasLocalPrimarySelectedUnitStatus = false;

	UPROPERTY(Transient)
	TArray<FSelectionSummaryItemViewModel> LocalSelectionSummaryItems;

	FVector ClickStartLocation = FVector::ZeroVector;
	
	bool bHasValidMousePositionThisFrame = false;
	FVector2D LastValidMouseScreenPosition = FVector2D::ZeroVector;      // 커서 표시용 (DPI 보정)
	FVector2D CurrentFrameMouseScreenPosition = FVector2D::ZeroVector;   // 커서 표시용 (DPI 보정)

	bool bHasValidRawMousePositionThisFrame = false;
	FVector2D CurrentFrameRawMousePosition = FVector2D::ZeroVector;      // 엣지 스크롤용 (raw)
	bool bWasEdgeScrollingLastFrame = false;
	
	bool bEdgeScrollLeftActive = false;
	bool bEdgeScrollRightActive = false;
	bool bEdgeScrollTopActive = false;
	bool bEdgeScrollBottomActive = false;
};