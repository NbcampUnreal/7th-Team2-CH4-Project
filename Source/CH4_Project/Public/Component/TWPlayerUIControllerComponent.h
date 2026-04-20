#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TWPlayerUIControllerComponent.generated.h"

class ATWPlayerController;
class UTWPlayerUIBridge;
class UTWHUDRootWidget;
class UDataTable;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CH4_PROJECT_API UTWPlayerUIControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTWPlayerUIControllerComponent();

	virtual void TickComponent(
		float DeltaTime,
		enum ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	void InitializeUI(
		ATWPlayerController* InOwnerController,
		TSubclassOf<UTWHUDRootWidget> InHUDRootWidgetClass,
		UDataTable* InCommandMetaTable,
		UDataTable* InSelectionPresentationTable,
		bool bInUseUIDebugFallback,
		FName InDefaultSelectedUnitId,
		FName InDefaultSelectedBuildingId,
		FName InDefaultMultiSelectedUnitId
	);

	void RefreshUI();
	void ForceRefreshSelectionBridge();

	void UpdateInputOverlayState(FName ArmedCommandId);
	void UpdateDragSelectionOverlay(
		bool bIsDragging,
		const FVector2D& DragStartScreenPosition,
		const FVector2D& CurrentMouseScreenPosition
	);
	void UpdateCursorOverlayPosition(
		const FVector2D& CursorScreenPosition,
		bool bEdgeScrollingActive
	);

	void HandleCommandRequested(FName CommandId);
	void RefreshBuildModeNotification();
	bool IsPointerOverHUD() const;
	UTWPlayerUIBridge* GetPlayerUIBridge() const { return PlayerUIBridge; }

protected:
	UPROPERTY()
	TObjectPtr<ATWPlayerController> OwnerController = nullptr;

	UPROPERTY()
	TObjectPtr<UTWPlayerUIBridge> PlayerUIBridge = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<UTWHUDRootWidget> HUDRootWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CommandMetaTable = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> SelectionPresentationTable = nullptr;

	UPROPERTY(Transient)
	bool bUseUIDebugFallback = false;

	UPROPERTY(Transient)
	FName DefaultSelectedUnitId = NAME_None;

	UPROPERTY(Transient)
	FName DefaultSelectedBuildingId = NAME_None;

	UPROPERTY(Transient)
	FName DefaultMultiSelectedUnitId = NAME_None;

	UPROPERTY(EditAnywhere, Category="UI|Selection Refresh")
	bool bEnableRealtimeSelectionRefresh = true;

	UPROPERTY(EditAnywhere, Category="UI|Selection Refresh", meta=(ClampMin="0.01"))
	float SelectionRefreshInterval = 0.30f;

	float SelectionRefreshAccumulator = 0.0f;
};