#include "Component/TWPlayerUIControllerComponent.h"

#include "Core/TWPlayerController.h"
#include "UI/Core/TWPlayerUIBridge.h"
#include "UI/Data/TWUIDataTypes.h"
#include "UI/Widgets/TWHUDRootWidget.h"

UTWPlayerUIControllerComponent::UTWPlayerUIControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTWPlayerUIControllerComponent::TickComponent(
	float DeltaTime,
	enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bEnableRealtimeSelectionRefresh)
	{
		return;
	}

	if (!OwnerController || !OwnerController->IsLocalController())
	{
		return;
	}

	if (!PlayerUIBridge)
	{
		return;
	}

	SelectionRefreshAccumulator += DeltaTime;
	if (SelectionRefreshAccumulator < SelectionRefreshInterval)
	{
		return;
	}

	SelectionRefreshAccumulator = 0.0f;

	const bool bHasSelectedBuilding = (OwnerController->GetSelectedBuilding() != nullptr);
	const bool bHasSelectedUnits = (OwnerController->GetLocalSelectedUnitCount() > 0);

	if (!bHasSelectedBuilding && !bHasSelectedUnits)
	{
		return;
	}

	ForceRefreshSelectionBridge();
}

void UTWPlayerUIControllerComponent::InitializeUI(
	ATWPlayerController* InOwnerController,
	TSubclassOf<UTWHUDRootWidget> InHUDRootWidgetClass,
	UDataTable* InCommandMetaTable,
	UDataTable* InSelectionPresentationTable,
	bool bInUseUIDebugFallback,
	FName InDefaultSelectedUnitId,
	FName InDefaultSelectedBuildingId,
	FName InDefaultMultiSelectedUnitId
)
{
	OwnerController = InOwnerController;
	HUDRootWidgetClass = InHUDRootWidgetClass;
	CommandMetaTable = InCommandMetaTable;
	SelectionPresentationTable = InSelectionPresentationTable;
	bUseUIDebugFallback = bInUseUIDebugFallback;
	DefaultSelectedUnitId = InDefaultSelectedUnitId;
	DefaultSelectedBuildingId = InDefaultSelectedBuildingId;
	DefaultMultiSelectedUnitId = InDefaultMultiSelectedUnitId;

	SelectionRefreshAccumulator = 0.0f;

	if (!OwnerController || !OwnerController->IsLocalController())
	{
		return;
	}

	if (!PlayerUIBridge)
	{
		PlayerUIBridge = NewObject<UTWPlayerUIBridge>(this);
	}

	if (!PlayerUIBridge)
	{
		return;
	}

	PlayerUIBridge->Initialize(
		OwnerController,
		HUDRootWidgetClass,
		CommandMetaTable,
		SelectionPresentationTable,
		bUseUIDebugFallback,
		DefaultSelectedUnitId,
		DefaultSelectedBuildingId,
		DefaultMultiSelectedUnitId
	);

	PlayerUIBridge->GetOnUICommandRequestedDelegate().RemoveAll(this);
	PlayerUIBridge->GetOnUICommandRequestedDelegate().AddUObject(
		this,
		&UTWPlayerUIControllerComponent::HandleUICommandRequested
	);
}

void UTWPlayerUIControllerComponent::RefreshUI()
{
	if (!PlayerUIBridge)
	{
		return;
	}

	PlayerUIBridge->RefreshAll();
}

void UTWPlayerUIControllerComponent::RefreshBuildModeNotification()
{
	if (!PlayerUIBridge)
	{
		return;
	}

	PlayerUIBridge->RefreshBuildModeNotification();
}

void UTWPlayerUIControllerComponent::ForceRefreshSelectionBridge()
{
	if (PlayerUIBridge)
	{
		PlayerUIBridge->ForceRefreshSelectionFromGameplayEvent();
	}
	else
	{
		RefreshUI();
	}
}

void UTWPlayerUIControllerComponent::UpdateInputOverlayState(FName ArmedCommandId)
{
	if (!PlayerUIBridge)
	{
		return;
	}

	if (ArmedCommandId.IsNone())
	{
		PlayerUIBridge->ClearArmedCommandState();
	}
	else
	{
		PlayerUIBridge->SetArmedCommandState(ArmedCommandId);
	}
}

void UTWPlayerUIControllerComponent::UpdateDragSelectionOverlay(
	bool bIsDragging,
	const FVector2D& DragStartScreenPosition,
	const FVector2D& CurrentMouseScreenPosition
)
{
	if (!PlayerUIBridge)
	{
		return;
	}

	PlayerUIBridge->SetDragSelectionState(
		bIsDragging,
		DragStartScreenPosition,
		CurrentMouseScreenPosition
	);
}

void UTWPlayerUIControllerComponent::UpdateCursorOverlayPosition(
	const FVector2D& CursorScreenPosition,
	bool bEdgeScrollingActive
)
{
	if (!PlayerUIBridge)
	{
		return;
	}

	PlayerUIBridge->SetCursorScreenPosition(CursorScreenPosition);
	PlayerUIBridge->SetEdgeScrollingActive(bEdgeScrollingActive);
}

void UTWPlayerUIControllerComponent::HandleUICommandRequested(FName CommandId)
{
	if (!OwnerController)
	{
		return;
	}

	OwnerController->HandleUICommandRequested(CommandId);
}

bool UTWPlayerUIControllerComponent::IsPointerOverHUD() const
{
	if (!PlayerUIBridge)
	{
		return false;
	}

	UTWHUDRootWidget* HUDRoot = PlayerUIBridge->GetHUDRootWidget();
	if (!HUDRoot)
	{
		return false;
	}

	return HUDRoot->IsHovered();
}