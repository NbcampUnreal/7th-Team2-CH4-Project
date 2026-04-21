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
class UTWHUDRootWidget;
class UTWPlayerUIControllerComponent;
class UTWPlayerSelectionVisualComponent;
class ATWHeroUnitBase;
class UUserWidget;
class UTWAlertWidget;

UCLASS()
class CH4_PROJECT_API ATWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATWPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Common = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_UnitCommand = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Build = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Debug = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_BuildingCommand = nullptr;

	bool bUnitCommandContextActive = false;
	bool bBuildContextActive = false;
	bool bBuildShortcutModeActive = false;
	bool bBuildingCommandContextActive = false;

	void SetMappingContextActive(UInputMappingContext* MappingContext, int32 Priority, bool bShouldBeActive, bool& bCurrentActive);
	void RefreshDynamicMappingContexts();
	bool ShouldUseUnitCommandContext() const;
	bool ShouldUseBuildContext() const;
	bool ShouldUseBuildingCommandContext() const;

#pragma region Input
protected:
	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LeftMouseAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RightMouseAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HoldCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScreenEdgeEnterMargin = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScreenEdgeExitMargin = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScrollSpeed = 1500.0f;

	void OnStartLeftMouseAction(const FInputActionValue& InputActionValue);
	void OnEndLeftMouseAction(const FInputActionValue& InputActionValue);
	void OnRightMouseAction(const FInputActionValue& InputActionValue);

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

#pragma region 병력 스폰 대기열 / 인구 수 대기열 / 연구소 대기열
protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyQAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyWAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyEAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyAAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeySAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyDAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyZAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyXAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> QueueHotkeyCAction = nullptr;

	void OnQueueHotkeyQ(const FInputActionValue& Value);
	void OnQueueHotkeyW(const FInputActionValue& Value);
	void OnQueueHotkeyE(const FInputActionValue& Value);
	void OnQueueHotkeyA(const FInputActionValue& Value);
	void OnQueueHotkeyS(const FInputActionValue& Value);
	void OnQueueHotkeyD(const FInputActionValue& Value);
	void OnQueueHotkeyZ(const FInputActionValue& Value);
	void OnQueueHotkeyX(const FInputActionValue& Value);
	void OnQueueHotkeyC(const FInputActionValue& Value);
	
	void HandleBuildingProductionSlot(int32 SlotIndex);
#pragma endregion

#pragma region 넥서스 데미지
protected:
	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputAction> IA_TestDamageBlockingBuilding = nullptr;

	UFUNCTION()
	void HandleTestDamageBlockingBuilding(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestDamageBlockingBuilding();
#pragma endregion

#pragma region 건설
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleBuildModeAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectWoodCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectStoneCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectPopulationCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectTroopCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectUpgradeCommandAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectBlockingCommandAction = nullptr;

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

	UFUNCTION(BlueprintCallable, Category="Input")
	bool IsBuildShortcutModeActive() const { return bBuildShortcutModeActive; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTWBuildComponent> BuildComponent = nullptr;
#pragma endregion

#pragma region UI / Visual Components
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTWPlayerUIControllerComponent> PlayerUIControllerComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTWPlayerSelectionVisualComponent> PlayerSelectionVisualComponent = nullptr;

public:
	void NotifyRecentCombatUnitDamaged(
		const FMassNetworkID& InUnitNetId,
		int32 InOwnerPlayerSlot,
		float InVisibleTime = 1.25f
	);

	UFUNCTION(Client, Reliable)
	void ClientNotifyRecentCombatUnitDamaged(
		const FMassNetworkID& InUnitNetId,
		int32 InOwnerPlayerSlot,
		float InVisibleTime
	);
	void NotifyRecentCombatBuildingDamaged(
	ATWBaseBuilding* InBuilding,
	float InVisibleTime = 1.25f
);

	UFUNCTION(Client, Reliable)
	void ClientNotifyRecentCombatBuildingDamaged(
		ATWBaseBuilding* InBuilding,
		float InVisibleTime
	);
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

	int32 LocalSelectedOwnerPlayerSlot = INDEX_NONE;
private:
	bool TryFindAttackableBuildingCandidate(
		const FVector& CommandLocation,
		int32 MyPlayerSlot,
		ATWBaseBuilding*& OutTargetBuilding,
		FVector& OutResolvedTargetLocation
	);

	bool TryResolveAttackableTargetPreview(
		const FVector& CommandLocation,
		int32 MyPlayerSlot,
		FMassEntityHandle& OutTargetEntity,
		ATWBaseBuilding*& OutTargetBuilding,
		FVector& OutResolvedTargetLocation
	);
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
		bool& bOutHasPrimaryHealth
	);

	void RefreshLocalSelectionRuntimeData();
	bool RefreshLocalPrimarySelectedUnitStatus();

	const FUICommandMetaRow* FindCommandMetaRowFromTable(FName CommandId) const;

	bool CanExecuteCommand(const FUICommandMetaRow* CommandMeta, FName CommandId) const;
	bool TryHandleImmediateCommand(const FUICommandMetaRow* CommandMeta, FName CommandId);
	bool TryHandleArmedCommand(const FUICommandMetaRow* CommandMeta, FName CommandId);
	bool TryHandleServerRoutedCommand(const FUICommandMetaRow* CommandMeta, FName CommandId);
	bool TryResolveBuildingCategoryFromPayload(FName PayloadId, EBuildingCategory& OutCategory) const;

	void SetArmedCommandId(FName InCommandId);
	void ClearArmedCommandId();

	UFUNCTION(Client, Reliable)
	void ClientApplyUnitSelection(
		const TArray<FMassNetworkID>& InNetworkIds,
		float InPrimaryHealth,
		bool bInHasPrimaryHealth,
		int32 InSelectedOwnerPlayerSlot
	);

	UFUNCTION(Client, Reliable)
	void ClientApplyBuildingSelection(ATWBaseBuilding* InBuilding);

	UFUNCTION(Client, Reliable)
	void ClientClearSelection();

	bool TryInvokeBuildingProduceById(ATWBaseBuilding* TargetBuilding, FName UnitId) const;
	bool TryInvokeBuildingProduceFallback(ATWBaseBuilding* TargetBuilding) const;

public:
	UFUNCTION()
	void HandleCommandById(FName CommandId);

	UFUNCTION(Server, Reliable)
	void ServerHandleCommandById(FName CommandId);
public:
	ATWHeroUnitBase* GetOwnedHeroUnit() const;
	bool IsOwnedHeroCurrentlySelected() const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> VictoryWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> DefeatWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UTWAlertWidget> AlertWidgetClass;
	
public:
	UFUNCTION(Client, Reliable)
	void Client_ShowGameResult(int32 GameResult);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Menu = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MenuWidgetClass;
	
	UPROPERTY()
	UUserWidget* MenuWidgetInstance;
	
	void ToggleMenu();
	
public:
	UFUNCTION(Client, Reliable)
	void Client_ShowMenu(bool Open);
	
public:
	UFUNCTION(Server, Reliable)
	void Server_RequestDefeat();

	
	UFUNCTION(Client, Reliable)
	void Client_ShowAlertMessage(const FString& AlertMessage);
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
	void UpdateInputOverlayState();
	void UpdateDragSelectionOverlay();
	void UpdateCursorOverlayPosition();

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float DragSelectionScreenThreshold = 8.f;

	bool bIsLeftMousePressed = false;
	bool bIsDraggingSelectionVisual = false;
	bool bConsumeLeftMouseRelease = false;

	FVector2D DragStartScreenPosition = FVector2D::ZeroVector;
	FVector2D CurrentMouseScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(Transient)
	FName ArmedCommandId = NAME_None;

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
	FVector2D LastValidMouseScreenPosition = FVector2D::ZeroVector;
	FVector2D CurrentFrameMouseScreenPosition = FVector2D::ZeroVector;
	bool bHasValidMousePositionThisFrame = false;

	FVector2D LastValidRawMousePosition = FVector2D::ZeroVector;
	FVector2D CurrentFrameRawMousePosition = FVector2D::ZeroVector;
	bool bHasValidRawMousePositionThisFrame = false;
	bool bWasEdgeScrollingLastFrame = false;
	
#pragma region Cheat
	
public:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatAddResource(EResourceType ResourceType, int32 Amount);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_CheatTimeScale(float TimeMultiplier);
	
#pragma endregion
	
};