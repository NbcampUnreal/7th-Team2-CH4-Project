#include "UI/Core/TWPlayerUIBridge.h"

#include "Subsystems/TWUnitSubsystem.h"
#include "Data/TWUnitTableRowBase.h"
#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Building/TWBaseBuilding.h"
#include "Building/TWTroopSpawnBuilding.h"
#include "Building/TWPopulationBuilding.h"
#include "Building/TWBlockingBuilding.h"
#include "Building/TWResourceBuilding.h"
#include "UI/Core/TWUIHUDCoordinator.h"
#include "UI/Core/TWUISelectionProvider.h"
#include "UI/Core/TWUIResourceProvider.h"
#include "UI/Data/TWUIDataTypes.h"
#include "UI/Widgets/TWHUDRootWidget.h"
#include "Data/TWUnitStatus.h"
#include "Data/TWBuildingTypes.h"
#include "UObject/UnrealType.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "UI/Data/TWUIInputStateTypes.h"
#include "UI/Widgets/TWHUDRootWidget.h"

namespace TWCommandIds
{
	static const FName Move(TEXT("Move"));
	static const FName Attack(TEXT("Attack"));
	static const FName Hold(TEXT("Hold"));
	static const FName BuildMenu(TEXT("BuildMenu"));
	static const FName Back(TEXT("Back"));
}

namespace TWUIBridgeTopBarHelpers
{
	static void SetIntPropertyByNames(FTopBarViewModel& InOutVM, const TArray<FName>& CandidatePropertyNames, int32 InValue)
	{
		UScriptStruct* StructType = FTopBarViewModel::StaticStruct();
		if (!StructType)
		{
			return;
		}

		for (const FName& PropertyName : CandidatePropertyNames)
		{
			if (FIntProperty* IntProperty = FindFProperty<FIntProperty>(StructType, PropertyName))
			{
				IntProperty->SetPropertyValue_InContainer(&InOutVM, InValue);
				return;
			}
		}
	}

	static void SetStringLikePropertyByNames(FTopBarViewModel& InOutVM, const TArray<FName>& CandidatePropertyNames, const FString& InValue)
	{
		UScriptStruct* StructType = FTopBarViewModel::StaticStruct();
		if (!StructType)
		{
			return;
		}

		for (const FName& PropertyName : CandidatePropertyNames)
		{
			if (FStrProperty* StrProperty = FindFProperty<FStrProperty>(StructType, PropertyName))
			{
				StrProperty->SetPropertyValue_InContainer(&InOutVM, InValue);
				return;
			}

			if (FTextProperty* TextProperty = FindFProperty<FTextProperty>(StructType, PropertyName))
			{
				TextProperty->SetPropertyValue_InContainer(&InOutVM, FText::FromString(InValue));
				return;
			}
		}
	}
}

namespace TWUIBridgeBuildingHelpers
{
	static UObject* FindObjectPropertyValueByNames(const UObject* SourceObject, const TArray<FName>& CandidatePropertyNames)
	{
		if (!SourceObject)
		{
			return nullptr;
		}

		const UStruct* StructType = SourceObject->GetClass();
		for (const FName& PropertyName : CandidatePropertyNames)
		{
			if (const FObjectProperty* ObjectProperty = FindFProperty<FObjectProperty>(StructType, PropertyName))
			{
				return ObjectProperty->GetObjectPropertyValue_InContainer(SourceObject);
			}
		}

		return nullptr;
	}

	static bool FindNamePropertyValueByNames(const UObject* SourceObject, const TArray<FName>& CandidatePropertyNames, FName& OutValue)
	{
		OutValue = NAME_None;

		if (!SourceObject)
		{
			return false;
		}

		const UStruct* StructType = SourceObject->GetClass();
		for (const FName& PropertyName : CandidatePropertyNames)
		{
			if (const FNameProperty* NameProperty = FindFProperty<FNameProperty>(StructType, PropertyName))
			{
				OutValue = NameProperty->GetPropertyValue_InContainer(SourceObject);
				return !OutValue.IsNone();
			}
		}

		return false;
	}

	static bool FindStringLikePropertyValueByNames(const UObject* SourceObject, const TArray<FName>& CandidatePropertyNames, FString& OutValue)
	{
		OutValue.Reset();

		if (!SourceObject)
		{
			return false;
		}

		const UStruct* StructType = SourceObject->GetClass();
		for (const FName& PropertyName : CandidatePropertyNames)
		{
			if (const FStrProperty* StrProperty = FindFProperty<FStrProperty>(StructType, PropertyName))
			{
				OutValue = StrProperty->GetPropertyValue_InContainer(SourceObject);
				return !OutValue.IsEmpty();
			}

			if (const FTextProperty* TextProperty = FindFProperty<FTextProperty>(StructType, PropertyName))
			{
				OutValue = TextProperty->GetPropertyValue_InContainer(SourceObject).ToString();
				return !OutValue.IsEmpty();
			}

			if (const FNameProperty* NameProperty = FindFProperty<FNameProperty>(StructType, PropertyName))
			{
				const FName NameValue = NameProperty->GetPropertyValue_InContainer(SourceObject);
				if (!NameValue.IsNone())
				{
					OutValue = NameValue.ToString();
					return true;
				}
			}
		}

		return false;
	}

	static bool FindNameArrayPropertyValueByNames(const UObject* SourceObject, const TArray<FName>& CandidatePropertyNames, TArray<FName>& OutValues)
	{
		OutValues.Empty();

		if (!SourceObject)
		{
			return false;
		}

		const UStruct* StructType = SourceObject->GetClass();
		for (const FName& PropertyName : CandidatePropertyNames)
		{
			if (const FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(StructType, PropertyName))
			{
				const FNameProperty* InnerNameProperty = CastField<FNameProperty>(ArrayProperty->Inner);
				if (!InnerNameProperty)
				{
					continue;
				}

				FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(SourceObject));
				for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
				{
					const void* ElementPtr = ArrayHelper.GetRawPtr(Index);
					const FName UnitId = InnerNameProperty->GetPropertyValue(ElementPtr);
					if (!UnitId.IsNone())
					{
						OutValues.Add(UnitId);
					}
				}

				return OutValues.Num() > 0;
			}
		}

		return false;
	}

	static UObject* ResolveBuildingDataAsset(const ATWBaseBuilding* Building)
	{
		static const TArray<FName> CandidateNames = {
			TEXT("BuildingDataAsset"),
			TEXT("BuildingData"),
			TEXT("BuildingInfo"),
			TEXT("DataAsset")
		};

		return FindObjectPropertyValueByNames(Building, CandidateNames);
	}

	static FName ResolveBuildingSelectionIdFromDataAssetOrFallback(const ATWBaseBuilding* Building, FName FallbackSelectionId)
	{
		if (const UObject* BuildingDataAsset = ResolveBuildingDataAsset(Building))
		{
			FName DataDrivenSelectionId = NAME_None;
			if (FindNamePropertyValueByNames(BuildingDataAsset, { TEXT("BuildingId") }, DataDrivenSelectionId))
			{
				return DataDrivenSelectionId;
			}
		}

		return FallbackSelectionId;
	}

	static FString ResolveBuildingDisplayNameFromDataAssetOrFallback(const ATWBaseBuilding* Building)
	{
		if (!Building)
		{
			return TEXT("");
		}

		if (const UObject* BuildingDataAsset = ResolveBuildingDataAsset(Building))
		{
			FString DataDrivenName;
			if (FindStringLikePropertyValueByNames(BuildingDataAsset, { TEXT("BuildingName") }, DataDrivenName))
			{
				return DataDrivenName;
			}
		}

		return Building->GetName();
	}

	static bool ResolveTrainableUnitIds(const ATWBaseBuilding* Building, TArray<FName>& OutTrainableUnitIds)
	{
		OutTrainableUnitIds.Empty();

		if (const UObject* BuildingDataAsset = ResolveBuildingDataAsset(Building))
		{
			return FindNameArrayPropertyValueByNames(BuildingDataAsset, { TEXT("TrainableUnitIds") }, OutTrainableUnitIds);
		}

		return false;
	}

	static const FUICommandMetaRow* FindProduceUnitCommandMetaByPayload(UDataTable* CommandMetaTable, FName UnitId)
	{
		if (!CommandMetaTable || UnitId.IsNone())
		{
			return nullptr;
		}

		static const FString ContextString(TEXT("TWUIBridgeBuildingHelpers::FindProduceUnitCommandMetaByPayload"));

		TArray<FUICommandMetaRow*> AllRows;
		CommandMetaTable->GetAllRows<FUICommandMetaRow>(ContextString, AllRows);

		for (const FUICommandMetaRow* Row : AllRows)
		{
			if (!Row)
			{
				continue;
			}

			if (Row->CommandType != ETWCommandType::ProduceUnit)
			{
				continue;
			}

			if (Row->PayloadId == UnitId)
			{
				return Row;
			}
		}

		return nullptr;
	}
}

void UTWPlayerUIBridge::Initialize(
	ATWPlayerController* InOwnerController,
	TSubclassOf<UTWHUDRootWidget> InHUDRootWidgetClass,
	UDataTable* InCommandMetaTable,
	UDataTable* InSelectionPresentationTable,
	bool bInUseDebugFallback,
	FName InDefaultSelectedUnitId,
	FName InDefaultSelectedBuildingId,
	FName InDefaultMultiSelectedUnitId
)
{
	OwnerController = InOwnerController;
	HUDRootWidgetClass = InHUDRootWidgetClass;
	CommandMetaTable = InCommandMetaTable;
	SelectionPresentationTable = InSelectionPresentationTable;
	bUseDebugFallback = bInUseDebugFallback;
	DefaultSelectedUnitId = InDefaultSelectedUnitId;
	DefaultSelectedBuildingId = InDefaultSelectedBuildingId;
	DefaultMultiSelectedUnitId = InDefaultMultiSelectedUnitId;

	ResetContext();

	if (!OwnerController || !HUDRootWidgetClass)
	{
		return;
	}

	if (!HUDRootWidget)
	{
		HUDRootWidget = CreateWidget<UTWHUDRootWidget>(OwnerController, HUDRootWidgetClass);
		if (HUDRootWidget)
		{
			HUDRootWidget->AddToViewport();
		}
	}

	if (!SelectionProvider)
	{
		SelectionProvider = NewObject<UTWUISelectionProvider>(this);
		if (SelectionProvider)
		{
			SelectionProvider->Initialize(OwnerController);
			SelectionProvider->SetUseDebugFallback(bUseDebugFallback);
		}
	}

	if (!ResourceProvider)
	{
		ResourceProvider = NewObject<UTWUIResourceProvider>(this);
		if (ResourceProvider)
		{
			ResourceProvider->Initialize(OwnerController);
			ResourceProvider->SetUseDebugFallback(bUseDebugFallback);
		}
	}

	if (!HUDCoordinator)
	{
		HUDCoordinator = NewObject<UTWUIHUDCoordinator>(this);
	}

	if (HUDCoordinator && HUDRootWidget && SelectionProvider && ResourceProvider)
	{
		HUDCoordinator->Initialize(
			HUDRootWidget,
			SelectionProvider,
			ResourceProvider,
			CommandMetaTable,
			SelectionPresentationTable
		);

		HUDCoordinator->GetOnCommandRequestedDelegate().RemoveAll(this);
		HUDCoordinator->GetOnCommandRequestedDelegate().AddUObject(
			this,
			&UTWPlayerUIBridge::HandleHUDCommandRequested
		);
	}
}

void UTWPlayerUIBridge::RefreshAll()
{
	RefreshSelection();
	RefreshResources();

	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshAll();
	}
	RefreshInputState();
	RefreshDragSelectionState();

	if (HUDRootWidget)
	{
		HUDRootWidget->SetCursorScreenPosition(CursorScreenPosition);
	}
}

bool UTWPlayerUIBridge::HandleContextCommand(FName CommandId)
{
	if (!SelectionProvider || CommandId.IsNone())
	{
		return false;
	}

	const FUICommandMetaRow* CommandMetaRow = FindCommandMetaRow(CommandId);
	if (!CommandMetaRow)
	{
		return false;
	}

	if (CommandMetaRow->CommandRoute != ETWCommandRoute::Context)
	{
		return false;
	}
	const bool bIsPureContextCommand =
		(CommandMetaRow->CommandType == ETWCommandType::OpenContext) ||
		(CommandMetaRow->CommandType == ETWCommandType::Back) ||
		CommandMetaRow->bReturnToPreviousContext;

	if (!bIsPureContextCommand)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UIBridge] Context-route command bypassed to gameplay: CommandId=%s / Type=%d / Route=%d"),
			*CommandId.ToString(),
			static_cast<int32>(CommandMetaRow->CommandType),
			static_cast<int32>(CommandMetaRow->CommandRoute)
		);
		return false;
	}

	if (CommandMetaRow->CommandType == ETWCommandType::OpenContext)
	{
		PushContext(CommandMetaRow->NextContext);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[UIBridge] OpenContext handled: CommandId=%s / NextContext=%s"),
			*CommandId.ToString(),
			*CommandMetaRow->NextContext.ToString()
		);
	}
	else if (CommandMetaRow->CommandType == ETWCommandType::Back || CommandMetaRow->bReturnToPreviousContext)
	{
		PopContext();

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[UIBridge] Back handled: CommandId=%s / CurrentContext=%s"),
			*CommandId.ToString(),
			*CurrentContextId.ToString()
		);
	}

	SelectionProvider->HandleCommandInput(CommandId, CommandMetaRow);
	return true;
}

void UTWPlayerUIBridge::HandleHUDCommandRequested(FName CommandId)
{
	UE_LOG(LogTemp, Log, TEXT("[UIBridge] HUD command requested: %s"), *CommandId.ToString());
	OnUICommandRequested.Broadcast(CommandId);
}

const FUICommandMetaRow* UTWPlayerUIBridge::FindCommandMetaRow(FName CommandId) const
{
	if (!HUDCoordinator || CommandId.IsNone())
	{
		return nullptr;
	}

	return HUDCoordinator->FindCommandMetaRow(CommandId);
}

bool UTWPlayerUIBridge::TryResolveCommandIdFromHotkey(const FKey& InKey, FName& OutCommandId) const
{
	OutCommandId = NAME_None;

	const FString TargetLabel = NormalizeHotkeyLabelFromKey(InKey);
	if (TargetLabel.IsEmpty())
	{
		return false;
	}

	for (const FName& CommandId : CurrentVisibleCommandIds)
	{
		const FUICommandMetaRow* CommandMetaRow = FindCommandMetaRow(CommandId);
		if (!CommandMetaRow)
		{
			continue;
		}

		const FString CommandHotkey = CommandMetaRow->HotkeyLabel.ToString().TrimStartAndEnd();
		if (CommandHotkey.Equals(TargetLabel, ESearchCase::IgnoreCase))
		{
			OutCommandId = CommandId;
			return true;
		}
	}

	return false;
}

void UTWPlayerUIBridge::ResetContext()
{
	ContextStack.Empty();
	CurrentContextId = NAME_None;
}

void UTWPlayerUIBridge::PushContext(FName InContextId)
{
	if (InContextId.IsNone())
	{
		return;
	}

	ContextStack.Add(InContextId);
	CurrentContextId = InContextId;
}

bool UTWPlayerUIBridge::PopContext()
{
	if (ContextStack.IsEmpty())
	{
		CurrentContextId = NAME_None;
		return false;
	}

	ContextStack.Pop();

	if (ContextStack.IsEmpty())
	{
		CurrentContextId = NAME_None;
	}
	else
	{
		CurrentContextId = ContextStack.Last();
	}

	return true;
}

void UTWPlayerUIBridge::ForceRefreshSelectionFromGameplayEvent()
{
	RefreshSelection();

	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshSelectionPanel();
		HUDCoordinator->RefreshCommandPanel();
	}

	StartPostCommandSelectionRefreshWindow();
}

void UTWPlayerUIBridge::StartProductionSelectionRefresh()
{
	if (bProductionSelectionRefreshActive || !OwnerController)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		ProductionSelectionRefreshTimerHandle,
		this,
		&UTWPlayerUIBridge::HandleProductionSelectionRefreshTick,
		0.1f,
		true
	);

	bProductionSelectionRefreshActive = true;

	UE_LOG(LogTemp, Log, TEXT("[UIBridge] StartProductionSelectionRefresh"));
}

void UTWPlayerUIBridge::StopProductionSelectionRefresh()
{
	if (!bProductionSelectionRefreshActive || !OwnerController)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(ProductionSelectionRefreshTimerHandle);
	bProductionSelectionRefreshActive = false;

	UE_LOG(LogTemp, Log, TEXT("[UIBridge] StopProductionSelectionRefresh"));
}

void UTWPlayerUIBridge::HandleProductionSelectionRefreshTick()
{
	if (!OwnerController)
	{
		StopProductionSelectionRefresh();
		return;
	}

	ATWBaseBuilding* SelectedBuilding = OwnerController->GetSelectedBuilding();
	ATWTroopSpawnBuilding* TroopBuilding = Cast<ATWTroopSpawnBuilding>(SelectedBuilding);

	if (!TroopBuilding || !TroopBuilding->IsProducing())
	{
		StopProductionSelectionRefresh();

		RefreshSelection();
		if (HUDCoordinator)
		{
			HUDCoordinator->RefreshSelectionPanel();
			HUDCoordinator->RefreshCommandPanel();
		}
		return;
	}

	RefreshSelection();

	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshSelectionPanel();
	}
}

void UTWPlayerUIBridge::StartPostCommandSelectionRefreshWindow()
{
	if (!OwnerController)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	PostCommandSelectionRefreshRemaining = 1.5f;

	if (!bPostCommandSelectionRefreshActive)
	{
		World->GetTimerManager().SetTimer(
			PostCommandSelectionRefreshTimerHandle,
			this,
			&UTWPlayerUIBridge::HandlePostCommandSelectionRefreshTick,
			0.1f,
			true
		);

		bPostCommandSelectionRefreshActive = true;
	}

	UE_LOG(LogTemp, Log, TEXT("[UIBridge] StartPostCommandSelectionRefreshWindow"));
}

void UTWPlayerUIBridge::StopPostCommandSelectionRefreshWindow()
{
	if (!OwnerController || !bPostCommandSelectionRefreshActive)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(PostCommandSelectionRefreshTimerHandle);
	bPostCommandSelectionRefreshActive = false;
	PostCommandSelectionRefreshRemaining = 0.f;

	UE_LOG(LogTemp, Log, TEXT("[UIBridge] StopPostCommandSelectionRefreshWindow"));
}

void UTWPlayerUIBridge::HandlePostCommandSelectionRefreshTick()
{
	if (!OwnerController)
	{
		StopPostCommandSelectionRefreshWindow();
		return;
	}

	PostCommandSelectionRefreshRemaining -= 0.1f;

	RefreshSelection();

	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshSelectionPanel();
		HUDCoordinator->RefreshCommandPanel();
	}

	ATWBaseBuilding* SelectedBuilding = OwnerController->GetSelectedBuilding();
	ATWTroopSpawnBuilding* TroopBuilding = Cast<ATWTroopSpawnBuilding>(SelectedBuilding);

	if (TroopBuilding && (TroopBuilding->IsProducing() || TroopBuilding->GetQueuedUnitIds().Num() > 0))
	{
		StopPostCommandSelectionRefreshWindow();
		return;
	}

	if (PostCommandSelectionRefreshRemaining <= 0.f)
	{
		StopPostCommandSelectionRefreshWindow();
	}
}

void UTWPlayerUIBridge::RefreshSelection()
{
	if (!SelectionProvider || !OwnerController)
	{
		return;
	}

	CurrentVisibleCommandIds.Empty();

	ATWBaseBuilding* SelectedBuilding = OwnerController->GetSelectedBuilding();

	if (SelectedBuilding)
	{
		const ATWPlayerState* LocalPS = OwnerController->GetPlayerState<ATWPlayerState>();

		const bool bIsOwnedByMe =
			LocalPS &&
			(SelectedBuilding->GetOwnerPlayerSlot() == LocalPS->PlayerSlot);

		FSelectionViewModel VM;
		VM.SelectionType = ESelectionViewType::Building;
		VM.ViewMode = ESelectionViewMode::Single;
		VM.SelectionId = TWUIBridgeBuildingHelpers::ResolveBuildingSelectionIdFromDataAssetOrFallback(
			SelectedBuilding,
			OwnerController->ResolveBuildingSelectionId(SelectedBuilding)
		);
		VM.DisplayName = TWUIBridgeBuildingHelpers::ResolveBuildingDisplayNameFromDataAssetOrFallback(SelectedBuilding);
		VM.TypeLabel = TEXT("Building");
		VM.TotalSelectedCount = 1;
		VM.CountLabel = TEXT("");
		VM.bShowProductionPanel = false;
		VM.Production = FBuildingProductionViewModel();

		// 건물 기본 정보
		VM.CurrentHP = SelectedBuilding->GetCurrentHP();
		VM.MaxHP = SelectedBuilding->GetMaxHP();
		VM.HPText = FString::Printf(TEXT("%.0f / %.0f"), VM.CurrentHP, VM.MaxHP);

		TArray<FName> CommandIds;

		if (ATWTroopSpawnBuilding* TroopBuilding = Cast<ATWTroopSpawnBuilding>(SelectedBuilding))
		{
			if (bIsOwnedByMe)
			{
				TArray<FName> TrainableUnitIds;
				TWUIBridgeBuildingHelpers::ResolveTrainableUnitIds(TroopBuilding, TrainableUnitIds);

				for (const FName& UnitId : TrainableUnitIds)
				{
					if (const FUICommandMetaRow* CommandMeta =
						TWUIBridgeBuildingHelpers::FindProduceUnitCommandMetaByPayload(CommandMetaTable, UnitId))
					{
						CommandIds.Add(CommandMeta->CommandId);
					}
				}

				VM.bShowProductionPanel = true;
				VM.Production.bVisible = true;
				VM.Production.Title = TEXT("생산 중");
				VM.Production.ProgressRatio = TroopBuilding->GetCurrentProductionProgressRatio();
				VM.Production.ProgressText = TroopBuilding->GetCurrentProductionProgressText();

				const TArray<FName>& QueuedUnitIds = TroopBuilding->GetQueuedUnitIds();

				for (int32 Index = 0; Index < QueuedUnitIds.Num(); ++Index)
				{
					const FName QueuedUnitId = QueuedUnitIds[Index];

					FProductionQueueItemViewModel QueueItem;
					QueueItem.PayloadId = QueuedUnitId;
					QueueItem.DisplayName = QueuedUnitId.IsNone() ? TEXT("Queued Unit") : QueuedUnitId.ToString();
					QueueItem.bIsActive = (Index == 0 && TroopBuilding->IsProducing());
					QueueItem.StackCount = 1;

					if (!QueuedUnitId.IsNone())
					{
						if (const FUICommandMetaRow* QueueCommandMeta =
							TWUIBridgeBuildingHelpers::FindProduceUnitCommandMetaByPayload(CommandMetaTable, QueuedUnitId))
						{
							QueueItem.Icon = QueueCommandMeta->Icon;

							if (!QueueCommandMeta->DisplayName.IsEmpty())
							{
								QueueItem.DisplayName = QueueCommandMeta->DisplayName.ToString();
							}
						}
					}

					VM.Production.QueueItems.Add(QueueItem);
				}

				if (TroopBuilding->IsProducing())
				{
					StartProductionSelectionRefresh();
				}
				else
				{
					StopProductionSelectionRefresh();
				}
			}
			else
			{
				// 상대 병력 생산 건물: 정보만 표시
				VM.bShowProductionPanel = false;
				VM.Production = FBuildingProductionViewModel();
				CommandIds.Reset();

				StopProductionSelectionRefresh();
				StopPostCommandSelectionRefreshWindow();
			}
		}
		else
		{
			StopProductionSelectionRefresh();
			StopPostCommandSelectionRefreshWindow();

			if (!bIsOwnedByMe)
			{
				// 상대 건물: 정보만 표시
				CommandIds.Reset();
			}
			else if (Cast<ATWPopulationBuilding>(SelectedBuilding))
			{
				CommandIds = {
					TWCommandIds::BuildMenu
				};
			}
			else if (Cast<ATWResourceBuilding>(SelectedBuilding) || Cast<ATWBlockingBuilding>(SelectedBuilding))
			{
				CommandIds = {};
			}
		}

			CurrentVisibleCommandIds = CommandIds;

			SelectionProvider->SetRuntimeSelection(
				VM,
				CommandIds,
				ESelectionViewMode::Single,
				VM.SelectionId,
				1,
				TArray<FSelectionSummaryItemViewModel>()
			);

		SelectionProvider->ClearRuntimeCommandData();

		for (const FName& CommandId : CommandIds)
		{
			const int32 QueueCount = ResolveBuildingQueueCount(CommandId);
			SelectionProvider->SetRuntimeCommandQueueCount(CommandId, QueueCount);
		}

		return;
	}

	StopProductionSelectionRefresh();
	StopPostCommandSelectionRefreshWindow();

	const int32 LocalSelectedUnitCount = OwnerController->GetLocalSelectedUnitCount();
	const FName LocalPrimarySelectedUnitId = OwnerController->GetLocalPrimarySelectedUnitId();
	const TArray<FSelectionSummaryItemViewModel>& LocalSummaryItems = OwnerController->GetLocalSelectionSummaryItems();

	if (LocalSelectedUnitCount <= 0)
	{
		SelectionProvider->ClearRuntimeSelection();
		SelectionProvider->ClearRuntimeCommandData();
		ResetContext();
		return;
	}

	const bool bIsMultiSelection = LocalSelectedUnitCount > 1;
	
	const ATWPlayerState* LocalPS = OwnerController->GetPlayerState<ATWPlayerState>();

	const bool bLocalUnitSelectionOwnedByMe =
		LocalPS &&
		(OwnerController->GetLocalSelectedOwnerPlayerSlot() == LocalPS->PlayerSlot);
	
	FSelectionViewModel VM;
	VM.SelectionType = bIsMultiSelection ? ESelectionViewType::Multi : ESelectionViewType::Unit;
	VM.ViewMode = bIsMultiSelection ? ESelectionViewMode::Multi : ESelectionViewMode::Single;
	VM.SelectionId = LocalPrimarySelectedUnitId.IsNone()
		? (bIsMultiSelection ? DefaultMultiSelectedUnitId : DefaultSelectedUnitId)
		: LocalPrimarySelectedUnitId;

	VM.DisplayName = bIsMultiSelection ? TEXT("Selected Units") : TEXT("");
	VM.TypeLabel = bIsMultiSelection ? TEXT("Units") : TEXT("");
	VM.TotalSelectedCount = LocalSelectedUnitCount;
	VM.CountLabel = bIsMultiSelection
		? FString::Printf(TEXT("%d Selected"), LocalSelectedUnitCount)
		: TEXT("");
	VM.bShowProductionPanel = false;
	VM.Production = FBuildingProductionViewModel();

	if (bIsMultiSelection)
	{
		VM.SummaryItems = LocalSummaryItems;
	}
	else
	{
		float CurrentHP = 0.f;
		float MaxHP = 0.f;
		bool bHasAnyHP = false;

		if (UTWUnitSubsystem* UnitSubsystem = OwnerController->GetWorld()->GetSubsystem<UTWUnitSubsystem>())
		{
			if (FTWUnitTableRowBase* UnitRow = UnitSubsystem->GetUnitTableRowBase(VM.SelectionId))
			{
				MaxHP = UnitRow->BaseStatus.GetStatus(ETWStatusType::Health);
				if (MaxHP > 0.f)
				{
					bHasAnyHP = true;
				}
			}
		}

		if (OwnerController->HasLocalPrimarySelectedUnitStatus())
		{
			const FTWUnitStatus& Status = OwnerController->GetLocalPrimarySelectedUnitStatus();
			CurrentHP = Status.GetStatus(ETWStatusType::Health);

			if (MaxHP <= 0.f)
			{
				MaxHP = CurrentHP;
			}

			bHasAnyHP = true;
		}
		else if (MaxHP > 0.f)
		{
			CurrentHP = MaxHP;
			bHasAnyHP = true;
		}

		if (bHasAnyHP)
		{
			VM.CurrentHP = CurrentHP;
			VM.MaxHP = MaxHP;
			VM.HPText = FString::Printf(TEXT("%.0f / %.0f"), VM.CurrentHP, VM.MaxHP);
		}
	}

	TArray<FName> CommandIds;
	if (bLocalUnitSelectionOwnedByMe)
	{
		CommandIds.Add(TWCommandIds::Move);
		CommandIds.Add(TWCommandIds::Attack);
		CommandIds.Add(TWCommandIds::Hold);
	}

	CurrentVisibleCommandIds = CommandIds;

	SelectionProvider->SetRuntimeSelection(
		VM,
		CommandIds,
		VM.ViewMode,
		VM.SelectionId,
		LocalSelectedUnitCount,
		VM.SummaryItems
	);

	SelectionProvider->ClearRuntimeCommandData();
}

void UTWPlayerUIBridge::RefreshResources()
{
	if (!ResourceProvider || !OwnerController)
	{
		return;
	}

	FTopBarViewModel VM;
	int32 ElapsedSeconds = 0;

	int32 Wood = 0;
	int32 Gas = 0;
	int32 Mithril = 0;
	int32 CurrentPopulation = 0;
	int32 PendingPopulation = 0;
	int32 PopulationLimit = 0;
	int32 MaxPopulation = 0;

	if (const ATWPlayerState* TWPS = OwnerController->GetPlayerState<ATWPlayerState>())
	{
		Wood = TWPS->GetResourceAmount(EResourceType::Wood);
		Gas = TWPS->GetResourceAmount(EResourceType::Ore);
		Mithril = TWPS->GetResourceAmount(EResourceType::Mithril);
		CurrentPopulation = TWPS->GetCurrentPopulation();
		PendingPopulation = TWPS->GetPendingPopulation();
		PopulationLimit = TWPS->GetPopulationLimit();
		MaxPopulation = TWPS->GetMaxPopulation();
	}

	const int32 DisplayPopulation = CurrentPopulation + PendingPopulation;

	VM.Wood = Wood;
	VM.Gas = Gas;
	VM.Mithril = Mithril;
	VM.Population = DisplayPopulation;
	VM.PopulationText = FString::Printf(TEXT("%d / %d"), DisplayPopulation, PopulationLimit);

	TWUIBridgeTopBarHelpers::SetIntPropertyByNames(VM, { TEXT("CurrentPopulation"), TEXT("PopulationUsed") }, CurrentPopulation);
	TWUIBridgeTopBarHelpers::SetIntPropertyByNames(VM, { TEXT("PendingPopulation") }, PendingPopulation);
	TWUIBridgeTopBarHelpers::SetIntPropertyByNames(VM, { TEXT("PopulationLimit"), TEXT("PopulationCap") }, PopulationLimit);
	TWUIBridgeTopBarHelpers::SetIntPropertyByNames(VM, { TEXT("MaxPopulation") }, MaxPopulation);

	TWUIBridgeTopBarHelpers::SetStringLikePropertyByNames(
		VM,
		{ TEXT("PopulationText"), TEXT("PopulationLabel"), TEXT("PopulationDisplayText") },
		VM.PopulationText
	);

	ResourceProvider->DebugSetResources(VM, ElapsedSeconds);
}

int32 UTWPlayerUIBridge::ResolveBuildingQueueCount(FName CommandId) const
{
	if (!OwnerController || CommandId.IsNone())
	{
		return 0;
	}

	ATWBaseBuilding* SelectedBuilding = OwnerController->GetSelectedBuilding();
	if (!SelectedBuilding)
	{
		return 0;
	}

	const ATWTroopSpawnBuilding* TroopBuilding = Cast<ATWTroopSpawnBuilding>(SelectedBuilding);
	if (!TroopBuilding)
	{
		return 0;
	}

	const FUICommandMetaRow* CommandMeta = FindCommandMetaRow(CommandId);
	if (!CommandMeta)
	{
		return 0;
	}

	if (CommandMeta->CommandType != ETWCommandType::ProduceUnit)
	{
		return 0;
	}

	int32 MatchedCount = 0;
	const TArray<FName>& QueuedUnitIds = TroopBuilding->GetQueuedUnitIds();

	for (const FName& QueuedUnitId : QueuedUnitIds)
	{
		if (QueuedUnitId == CommandMeta->PayloadId)
		{
			++MatchedCount;
		}
	}

	return MatchedCount;
}

FString UTWPlayerUIBridge::NormalizeHotkeyLabelFromKey(const FKey& InKey) const
{
	if (!InKey.IsValid())
	{
		return TEXT("");
	}

	return InKey.GetDisplayName().ToString().TrimStartAndEnd().ToUpper();
}

void UTWPlayerUIBridge::SetArmedCommandState(FName InCommandId)
{
	ArmedCommandId = InCommandId;

	if (HUDCoordinator)
	{
		HUDCoordinator->SetArmedCommandId(ArmedCommandId);
	}

	RefreshInputState();
}

void UTWPlayerUIBridge::ClearArmedCommandState()
{
	ArmedCommandId = NAME_None;

	if (HUDCoordinator)
	{
		HUDCoordinator->SetArmedCommandId(NAME_None);
	}

	RefreshInputState();
}

void UTWPlayerUIBridge::SetDragSelectionState(bool bInVisible, const FVector2D& InStart, const FVector2D& InEnd)
{
	CachedDragSelectionState.bVisible = bInVisible;
	CachedDragSelectionState.StartScreenPosition = InStart;
	CachedDragSelectionState.EndScreenPosition = InEnd;

	RefreshDragSelectionState();
}

void UTWPlayerUIBridge::SetCursorScreenPosition(const FVector2D& InScreenPosition)
{
	CursorScreenPosition = InScreenPosition;

	if (HUDRootWidget)
	{
		HUDRootWidget->SetCursorScreenPosition(CursorScreenPosition);
	}
}

void UTWPlayerUIBridge::RefreshInputState()
{
	if (!HUDRootWidget)
	{
		return;
	}

	FUICommandInputStateViewModel VM;
	VM.bVisible = true;
	VM.ArmedCommandId = ArmedCommandId;
	VM.HintText = TEXT("");
	VM.CursorVisualType = ETWCursorVisualType::Default;

	// 1순위: 엣지 스크롤
	if (bEdgeScrollingActive)
	{
		VM.CursorVisualType = ETWCursorVisualType::EdgeScroll;
	}
	// 2순위: 명령 armed 상태
	else if (ArmedCommandId == TEXT("Move"))
	{
		VM.CursorVisualType = ETWCursorVisualType::Move;
		VM.HintText = TEXT("이동 위치를 클릭하세요. 우클릭으로 취소");
	}
	else if (ArmedCommandId == TEXT("Attack"))
	{
		VM.CursorVisualType = ETWCursorVisualType::Attack;
		VM.HintText = TEXT("공격 대상 또는 지점을 클릭하세요. 우클릭으로 취소");
	}
	else if (ArmedCommandId == TEXT("Build"))
	{
		VM.CursorVisualType = ETWCursorVisualType::Build;
		VM.HintText = TEXT("건설 위치를 지정하세요. 우클릭으로 취소");
	}
	else if (ArmedCommandId == TEXT("Forbidden"))
	{
		VM.CursorVisualType = ETWCursorVisualType::Forbidden;
		VM.HintText = TEXT("이 위치에는 배치할 수 없습니다.");
	}

	HUDRootWidget->SetInputStateData(VM);
	HUDRootWidget->SetCursorScreenPosition(CursorScreenPosition);
}

void UTWPlayerUIBridge::RefreshDragSelectionState()
{
	if (!HUDRootWidget)
	{
		return;
	}

	HUDRootWidget->SetDragSelectionStateData(CachedDragSelectionState);
}

void UTWPlayerUIBridge::SetCursorOverlayVisible(bool bInVisible)
{
	if (HUDRootWidget)
	{
		HUDRootWidget->SetCursorOverlayVisible(bInVisible);
	}
}

void UTWPlayerUIBridge::SetEdgeScrollingActive(bool bInActive)
{
	if (bEdgeScrollingActive == bInActive)
	{
		return;
	}

	bEdgeScrollingActive = bInActive;
	RefreshInputState();
}
