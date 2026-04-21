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
#include "Building/TWUpgradeBuilding.h"
#include "UI/Core/TWUIHUDCoordinator.h"
#include "UI/Core/TWUISelectionProvider.h"
#include "UI/Core/TWUIResourceProvider.h"
#include "UI/Data/TWUIDataTypes.h"
#include "UI/Widgets/TWHUDRootWidget.h"
#include "Data/TWUnitStatus.h"
#include "UObject/UnrealType.h"
#include "TimerManager.h"
#include "HeroUnit/TWHeroUnitBase.h"
#include "Engine/World.h"

namespace TWCommandIds
{
	static const FName Move(TEXT("Move"));
	static const FName Attack(TEXT("Attack"));
	static const FName Hold(TEXT("Hold"));
	static const FName BuildMenu(TEXT("BuildMenu"));
	static const FName Back(TEXT("Back"));
	static const FName IncreasePopulation(TEXT("IncreasePopulation"));
	static const FName HeroSkill(TEXT("HeroSkill"));
}

namespace TWUIBridgeText
{
	static const FString BuildModeDisplayName(TEXT("건설 모드"));
	static const FString BuildModeTypeLabel(TEXT("Build"));
	static const FString BuildingTypeLabel(TEXT("Building"));
	static const FString UnitTypeLabel(TEXT("Unit"));
	static const FString UnitsTypeLabel(TEXT("Units"));
	static const FString MultiSelectedUnits(TEXT("Selected Units"));
	static const FString ProducingTitle(TEXT("생산 중"));
	static const FString ResearchingTitle(TEXT("업그레이드 중"));
	static const FString IncreasePopulationDisplayName(TEXT("인구수 증가"));
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
					const FName Value = InnerNameProperty->GetPropertyValue(ElementPtr);
					if (!Value.IsNone())
					{
						OutValues.Add(Value);
					}
				}

				return OutValues.Num() > 0;
			}
		}

		return false;
	}

	static void AppendBuildModeCommandIds(
		UDataTable* CommandMetaTable,
		TArray<FName>& OutCommandIds)
	{
		OutCommandIds.Empty();

		if (!CommandMetaTable)
		{
			return;
		}

		static const FString ContextString(TEXT("TWUIBridgeBuildingHelpers::AppendBuildModeCommandIds"));

		TArray<FUICommandMetaRow*> AllRows;
		CommandMetaTable->GetAllRows<FUICommandMetaRow>(ContextString, AllRows);

		TArray<const FUICommandMetaRow*> SortedRows;
		SortedRows.Reserve(AllRows.Num());

		for (const FUICommandMetaRow* Row : AllRows)
		{
			if (!Row)
			{
				continue;
			}

			const bool bIsBuildStructure = (Row->CommandType == ETWCommandType::BuildStructure);
			const bool bIsBackCommand =
				(Row->CommandType == ETWCommandType::Back) ||
				(Row->CommandId == TWCommandIds::Back);

			if (!bIsBuildStructure && !bIsBackCommand)
			{
				continue;
			}

			SortedRows.Add(Row);
		}

		SortedRows.Sort(
			[](const FUICommandMetaRow& A, const FUICommandMetaRow& B)
			{
				if (A.SlotIndex == B.SlotIndex)
				{
					return A.CommandId.LexicalLess(B.CommandId);
				}
				return A.SlotIndex < B.SlotIndex;
			}
		);

		for (const FUICommandMetaRow* Row : SortedRows)
		{
			if (Row && !Row->CommandId.IsNone())
			{
				OutCommandIds.AddUnique(Row->CommandId);
			}
		}
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

	static const FUICommandMetaRow* FindUpgradeCommandMetaByPayload(UDataTable* CommandMetaTable, FName UpgradeId)
	{
		if (!CommandMetaTable || UpgradeId.IsNone())
		{
			return nullptr;
		}

		static const FString ContextString(TEXT("TWUIBridgeBuildingHelpers::FindUpgradeCommandMetaByPayload"));

		TArray<FUICommandMetaRow*> AllRows;
		CommandMetaTable->GetAllRows<FUICommandMetaRow>(ContextString, AllRows);

		for (const FUICommandMetaRow* Row : AllRows)
		{
			if (!Row)
			{
				continue;
			}

			if (Row->CommandType != ETWCommandType::Research)
			{
				continue;
			}

			if (Row->PayloadId == UpgradeId)
			{
				return Row;
			}
		}

		return nullptr;
	}

	static TSoftObjectPtr<UTexture2D> ResolveUpgradeQueueItemIcon(
		UDataTable* CommandMetaTable,
		FName QueuedUpgradeId)
	{
		if (QueuedUpgradeId.IsNone())
		{
			return nullptr;
		}

		if (const FUICommandMetaRow* CommandMeta =
			FindUpgradeCommandMetaByPayload(CommandMetaTable, QueuedUpgradeId))
		{
			if (!CommandMeta->Icon.IsNull())
			{
				return CommandMeta->Icon;
			}
		}

		return nullptr;
	}

	static const FUISelectionPresentationRow* FindSelectionPresentationRowByEntityId(
		UDataTable* SelectionPresentationTable,
		FName InEntityId)
	{
		if (!SelectionPresentationTable || InEntityId.IsNone())
		{
			return nullptr;
		}

		static const FString Context(TEXT("TWUIBridgeBuildingHelpers::FindSelectionPresentationRowByEntityId"));

		if (const FUISelectionPresentationRow* Row =
			SelectionPresentationTable->FindRow<FUISelectionPresentationRow>(InEntityId, Context))
		{
			return Row;
		}

		TArray<FUISelectionPresentationRow*> AllRows;
		SelectionPresentationTable->GetAllRows<FUISelectionPresentationRow>(Context, AllRows);

		for (const FUISelectionPresentationRow* Candidate : AllRows)
		{
			if (Candidate && Candidate->EntityId == InEntityId)
			{
				return Candidate;
			}
		}

		return nullptr;
	}

	static TSoftObjectPtr<UTexture2D> ResolveQueueItemIcon(
		UDataTable* CommandMetaTable,
		UDataTable* SelectionPresentationTable,
		FName QueuedUnitId)
	{
		if (QueuedUnitId.IsNone())
		{
			return nullptr;
		}

		if (const FUISelectionPresentationRow* PresentationRow =
			FindSelectionPresentationRowByEntityId(SelectionPresentationTable, QueuedUnitId))
		{
			if (!PresentationRow->PortraitIcon.IsNull())
			{
				return PresentationRow->PortraitIcon;
			}

			if (!PresentationRow->Portrait.IsNull())
			{
				return PresentationRow->Portrait;
			}
		}

		if (const FUICommandMetaRow* CommandMeta = FindProduceUnitCommandMetaByPayload(CommandMetaTable, QueuedUnitId))
		{
			if (!CommandMeta->Icon.IsNull())
			{
				return CommandMeta->Icon;
			}
		}

		return nullptr;
	}
}

namespace TWUIBridgeUnitHelpers
{
	static void FillUnitDetailStats(
		FSelectionViewModel& InOutVM,
		const FTWUnitStatus* RuntimeStatus,
		const FTWUnitStatus* BaseStatus
	)
	{
		const FTWUnitStatus* PreferredStatus = RuntimeStatus ? RuntimeStatus : BaseStatus;
		if (!PreferredStatus)
		{
			InOutVM.bShowDetailStats = false;
			InOutVM.Damage = 0.f;
			InOutVM.Armor = 0.f;
			InOutVM.AttackSpeed = 0.f;
			InOutVM.MoveSpeed = 0.f;
			InOutVM.Range = 0.f;
			return;
		}

		InOutVM.bShowDetailStats = true;
		InOutVM.Damage = PreferredStatus->GetStatus(ETWStatusType::Damage);
		InOutVM.Armor = PreferredStatus->GetStatus(ETWStatusType::Armor);
		InOutVM.AttackSpeed = PreferredStatus->GetStatus(ETWStatusType::AttackSpeed);
		InOutVM.MoveSpeed = PreferredStatus->GetStatus(ETWStatusType::MoveSpeed);
		InOutVM.Range = PreferredStatus->GetStatus(ETWStatusType::Range);
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

void UTWPlayerUIBridge::RefreshTopBarOnly()
{
	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshTopBar();
	}
}

int32 UTWPlayerUIBridge::GetCurrentElapsedSeconds() const
{
	if (!ResourceProvider)
	{
		return 0;
	}

	return ResourceProvider->GetElapsedSeconds();
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
	RefreshBuildModeNotification();

	if (HUDRootWidget)
	{
		HUDRootWidget->SetCursorScreenPosition(CursorScreenPosition);
	}
}

bool UTWPlayerUIBridge::HandleContextCommand(FName CommandId)
{
	// build mode / selection context는 PlayerController 상태를 source of truth로 사용
	// 기존 외부 호환을 위해 false만 반환
	return false;
}

void UTWPlayerUIBridge::HandleHUDCommandRequested(FName CommandId)
{
	UE_LOG(LogTemp, Log, TEXT("[UIBridge] HUD command requested: %s"), *CommandId.ToString());

	if (CommandId.IsNone())
	{
		return;
	}

	OnUICommandRequested.Broadcast(CommandId);

	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshSelectionPanel();
	}
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

bool UTWPlayerUIBridge::TryGetVisibleCommandIdAtIndex(int32 Index, FName& OutCommandId) const
{
	OutCommandId = NAME_None;

	if (!CurrentVisibleCommandIds.IsValidIndex(Index))
	{
		return false;
	}

	OutCommandId = CurrentVisibleCommandIds[Index];
	return !OutCommandId.IsNone();
}

void UTWPlayerUIBridge::ResetContext()
{
	// no-op
}

void UTWPlayerUIBridge::PushContext(FName InContextId)
{
	// no-op
}

bool UTWPlayerUIBridge::PopContext()
{
	// no-op
	return false;
}

void UTWPlayerUIBridge::ForceRefreshSelectionFromGameplayEvent()
{
	RefreshSelection();

	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshSelectionPanel();
	}
}

void UTWPlayerUIBridge::StartSelectionRefreshTimer(float InGraceSeconds)
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

	SelectionRefreshGraceRemaining = FMath::Max(SelectionRefreshGraceRemaining, InGraceSeconds);

	if (bSelectionRefreshTimerActive)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		SelectionRefreshTimerHandle,
		this,
		&UTWPlayerUIBridge::HandleSelectionRefreshTick,
		0.1f,
		true
	);

	bSelectionRefreshTimerActive = true;
}

void UTWPlayerUIBridge::StopSelectionRefreshTimer()
{
	if (!OwnerController || !bSelectionRefreshTimerActive)
	{
		SelectionRefreshGraceRemaining = 0.0f;
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(SelectionRefreshTimerHandle);
	bSelectionRefreshTimerActive = false;
	SelectionRefreshGraceRemaining = 0.0f;
}

bool UTWPlayerUIBridge::ShouldKeepSelectionRefreshTimerRunning() const
{
	if (!OwnerController)
	{
		return false;
	}

	ATWBaseBuilding* SelectedBuilding = OwnerController->GetSelectedBuilding();
	if (!SelectedBuilding)
	{
		return false;
	}

	if (const ATWTroopSpawnBuilding* TroopBuilding = Cast<ATWTroopSpawnBuilding>(SelectedBuilding))
	{
		return TroopBuilding->IsProducing() || TroopBuilding->GetQueuedUnitIds().Num() > 0;
	}

	if (const ATWPopulationBuilding* PopulationBuilding = Cast<ATWPopulationBuilding>(SelectedBuilding))
	{
		return PopulationBuilding->IsProducing() || PopulationBuilding->GetCurrentQueueCount() > 0;
	}

	if (const ATWUpgradeBuilding* UpgradeBuilding = Cast<ATWUpgradeBuilding>(SelectedBuilding))
	{
		return UpgradeBuilding->IsProducing() || UpgradeBuilding->GetQueuedUpgradeIds().Num() > 0;
	}

	return false;
}

void UTWPlayerUIBridge::UpdateSelectionRefreshPolicyFromSelectedBuilding()
{
	if (ShouldKeepSelectionRefreshTimerRunning())
	{
		StartSelectionRefreshTimer(0.0f);
	}
	else
	{
		StopSelectionRefreshTimer();
	}
}

void UTWPlayerUIBridge::HandleSelectionRefreshTick()
{
	if (!OwnerController)
	{
		StopSelectionRefreshTimer();
		return;
	}

	RefreshSelection();

	if (HUDCoordinator)
	{
		HUDCoordinator->RefreshSelectionPanel();
		HUDCoordinator->RefreshCommandPanel();
	}

	const bool bShouldKeepRunning = ShouldKeepSelectionRefreshTimerRunning();

	if (bShouldKeepRunning)
	{
		SelectionRefreshGraceRemaining = 0.0f;
		return;
	}

	SelectionRefreshGraceRemaining -= 0.1f;
	if (SelectionRefreshGraceRemaining <= 0.0f)
	{
		StopSelectionRefreshTimer();
	}
}

void UTWPlayerUIBridge::FillCommonBuildingSelectionVM(ATWBaseBuilding* SelectedBuilding, FSelectionViewModel& OutVM) const
{
	OutVM.SelectionType = ESelectionViewType::Building;
	OutVM.ViewMode = ESelectionViewMode::Single;
	OutVM.SelectionId = TWUIBridgeBuildingHelpers::ResolveBuildingSelectionIdFromDataAssetOrFallback(
		SelectedBuilding,
		OwnerController ? OwnerController->ResolveBuildingSelectionId(SelectedBuilding) : NAME_None
	);
	OutVM.DisplayName = TWUIBridgeBuildingHelpers::ResolveBuildingDisplayNameFromDataAssetOrFallback(SelectedBuilding);
	OutVM.TypeLabel = TWUIBridgeText::BuildingTypeLabel;
	OutVM.TotalSelectedCount = 1;
	OutVM.CountLabel = TEXT("");
	OutVM.bShowProductionPanel = false;
	OutVM.Production = FBuildingProductionViewModel();
	OutVM.bShowDetailStats = false;
	OutVM.Damage = 0.f;
	OutVM.Armor = 0.f;
	OutVM.AttackSpeed = 0.f;
	OutVM.MoveSpeed = 0.f;
	OutVM.Range = 0.f;
	OutVM.CurrentHP = SelectedBuilding ? SelectedBuilding->GetCurrentHP() : 0.f;
	OutVM.MaxHP = SelectedBuilding ? SelectedBuilding->GetMaxHP() : 0.f;
	OutVM.HPText = FString::Printf(TEXT("%.0f / %.0f"), OutVM.CurrentHP, OutVM.MaxHP);
}

void UTWPlayerUIBridge::FillTroopBuildingSelectionData(
	ATWTroopSpawnBuilding* TroopBuilding,
	FSelectionViewModel& InOutVM,
	TArray<FName>& OutCommandIds
)
{
	OutCommandIds.Reset();

	if (!TroopBuilding)
	{
		return;
	}

	TArray<FName> TrainableUnitIds;
	TWUIBridgeBuildingHelpers::ResolveTrainableUnitIds(TroopBuilding, TrainableUnitIds);

	for (const FName& UnitId : TrainableUnitIds)
	{
		if (const FUICommandMetaRow* CommandMeta =
			TWUIBridgeBuildingHelpers::FindProduceUnitCommandMetaByPayload(CommandMetaTable, UnitId))
		{
			OutCommandIds.Add(CommandMeta->CommandId);
		}
	}

	InOutVM.bShowProductionPanel = true;
	InOutVM.Production.bVisible = true;
	InOutVM.Production.Title = TWUIBridgeText::ProducingTitle;
	InOutVM.Production.ProgressRatio = TroopBuilding->GetCurrentProductionProgressRatio();
	InOutVM.Production.ProgressText = TroopBuilding->GetCurrentProductionProgressText();

	const TArray<FName>& QueuedUnitIds = TroopBuilding->GetQueuedUnitIds();

	for (int32 Index = 0; Index < QueuedUnitIds.Num(); ++Index)
	{
		const FName QueuedUnitId = QueuedUnitIds[Index];

		FProductionQueueItemViewModel QueueItem;
		QueueItem.PayloadId = QueuedUnitId;
		QueueItem.DisplayName = QueuedUnitId.IsNone() ? TEXT("Queued Unit") : QueuedUnitId.ToString();
		QueueItem.bIsActive = (Index == 0 && TroopBuilding->IsProducing());
		QueueItem.StackCount = 1;
		QueueItem.Icon = TWUIBridgeBuildingHelpers::ResolveQueueItemIcon(
			CommandMetaTable,
			SelectionPresentationTable,
			QueuedUnitId
		);

		if (!QueuedUnitId.IsNone())
		{
			if (const FUICommandMetaRow* QueueCommandMeta =
				TWUIBridgeBuildingHelpers::FindProduceUnitCommandMetaByPayload(CommandMetaTable, QueuedUnitId))
			{
				if (!QueueCommandMeta->DisplayName.IsEmpty())
				{
					QueueItem.DisplayName = QueueCommandMeta->DisplayName.ToString();
				}

				if (QueueItem.Icon.IsNull() && !QueueCommandMeta->Icon.IsNull())
				{
					QueueItem.Icon = QueueCommandMeta->Icon;
				}
			}
		}

		InOutVM.Production.QueueItems.Add(QueueItem);
	}

	UpdateSelectionRefreshPolicyFromSelectedBuilding();
}

void UTWPlayerUIBridge::FillUpgradeBuildingSelectionData(
	ATWUpgradeBuilding* UpgradeBuilding,
	FSelectionViewModel& InOutVM,
	TArray<FName>& OutCommandIds
)
{
	OutCommandIds.Reset();

	if (!UpgradeBuilding)
	{
		return;
	}

	TArray<FName> AvailableUpgradeIds;
	UpgradeBuilding->ResolveAvailableUpgradeIds(AvailableUpgradeIds);

	for (const FName& UpgradeId : AvailableUpgradeIds)
	{
		if (const FUICommandMetaRow* CommandMeta =
			TWUIBridgeBuildingHelpers::FindUpgradeCommandMetaByPayload(CommandMetaTable, UpgradeId))
		{
			OutCommandIds.Add(CommandMeta->CommandId);
		}
	}

	InOutVM.bShowProductionPanel = true;
	InOutVM.Production.bVisible = true;
	InOutVM.Production.Title = TWUIBridgeText::ResearchingTitle;
	InOutVM.Production.ProgressRatio = UpgradeBuilding->GetCurrentProductionProgressRatio();
	InOutVM.Production.ProgressText = UpgradeBuilding->GetCurrentProductionProgressText();

	const TArray<FName>& QueuedUpgradeIds = UpgradeBuilding->GetQueuedUpgradeIds();

	for (int32 Index = 0; Index < QueuedUpgradeIds.Num(); ++Index)
	{
		const FName QueuedUpgradeId = QueuedUpgradeIds[Index];

		FProductionQueueItemViewModel QueueItem;
		QueueItem.PayloadId = QueuedUpgradeId;
		QueueItem.DisplayName = QueuedUpgradeId.IsNone() ? TEXT("Queued Upgrade") : QueuedUpgradeId.ToString();
		QueueItem.bIsActive = (Index == 0 && UpgradeBuilding->IsProducing());
		QueueItem.StackCount = 1;
		QueueItem.Icon = TWUIBridgeBuildingHelpers::ResolveUpgradeQueueItemIcon(
			CommandMetaTable,
			QueuedUpgradeId
		);

		if (!QueuedUpgradeId.IsNone())
		{
			if (const FUICommandMetaRow* QueueCommandMeta =
				TWUIBridgeBuildingHelpers::FindUpgradeCommandMetaByPayload(CommandMetaTable, QueuedUpgradeId))
			{
				if (!QueueCommandMeta->DisplayName.IsEmpty())
				{
					QueueItem.DisplayName = QueueCommandMeta->DisplayName.ToString();
				}

				if (QueueItem.Icon.IsNull() && !QueueCommandMeta->Icon.IsNull())
				{
					QueueItem.Icon = QueueCommandMeta->Icon;
				}
			}
		}

		InOutVM.Production.QueueItems.Add(QueueItem);
	}

	UpdateSelectionRefreshPolicyFromSelectedBuilding();
}

void UTWPlayerUIBridge::FillPopulationBuildingSelectionData(
	ATWPopulationBuilding* PopulationBuilding,
	FSelectionViewModel& InOutVM,
	TArray<FName>& OutCommandIds
)
{
	OutCommandIds = { TWCommandIds::IncreasePopulation };

	if (!PopulationBuilding)
	{
		return;
	}

	InOutVM.bShowProductionPanel = true;
	InOutVM.Production.bVisible = true;
	InOutVM.Production.Title = TWUIBridgeText::ProducingTitle;
	InOutVM.Production.ProgressRatio = PopulationBuilding->GetCurrentProductionProgressRatio();
	InOutVM.Production.ProgressText = PopulationBuilding->GetCurrentProductionProgressText();

	TSoftObjectPtr<UTexture2D> PopulationQueueIcon = nullptr;
	if (const FUICommandMetaRow* QueueCommandMeta = FindCommandMetaRow(TWCommandIds::IncreasePopulation))
	{
		PopulationQueueIcon = QueueCommandMeta->Icon;
	}

	if (PopulationQueueIcon.IsNull())
	{
		if (const FUISelectionPresentationRow* PresentationRow =
			TWUIBridgeBuildingHelpers::FindSelectionPresentationRowByEntityId(SelectionPresentationTable, InOutVM.SelectionId))
		{
			if (!PresentationRow->PortraitIcon.IsNull())
			{
				PopulationQueueIcon = PresentationRow->PortraitIcon;
			}
			else if (!PresentationRow->Portrait.IsNull())
			{
				PopulationQueueIcon = PresentationRow->Portrait;
			}
		}
	}

	const int32 QueueCount = PopulationBuilding->GetCurrentQueueCount();

	for (int32 Index = 0; Index < QueueCount; ++Index)
	{
		FProductionQueueItemViewModel QueueItem;
		QueueItem.PayloadId = TWCommandIds::IncreasePopulation;
		QueueItem.DisplayName = TWUIBridgeText::IncreasePopulationDisplayName;
		QueueItem.bIsActive = (Index == 0 && PopulationBuilding->IsProducing());
		QueueItem.StackCount = 1;
		QueueItem.Icon = PopulationQueueIcon;

		if (const FUICommandMetaRow* QueueCommandMeta = FindCommandMetaRow(TWCommandIds::IncreasePopulation))
		{
			if (!QueueCommandMeta->DisplayName.IsEmpty())
			{
				QueueItem.DisplayName = QueueCommandMeta->DisplayName.ToString();
			}

			if (QueueItem.Icon.IsNull() && !QueueCommandMeta->Icon.IsNull())
			{
				QueueItem.Icon = QueueCommandMeta->Icon;
			}
		}

		InOutVM.Production.QueueItems.Add(QueueItem);
	}

	UpdateSelectionRefreshPolicyFromSelectedBuilding();
}

bool UTWPlayerUIBridge::TryBuildModeSelectionVM(FSelectionViewModel& OutVM, TArray<FName>& OutCommandIds)
{
	if (!OwnerController || !OwnerController->IsBuildShortcutModeActive())
	{
		return false;
	}

	StopSelectionRefreshTimer();

	OutVM = FSelectionViewModel();
	OutVM.SelectionType = ESelectionViewType::Building;
	OutVM.ViewMode = ESelectionViewMode::Single;
	OutVM.SelectionId = TEXT("BuildMenu");
	OutVM.DisplayName = TWUIBridgeText::BuildModeDisplayName;
	OutVM.TypeLabel = TWUIBridgeText::BuildModeTypeLabel;
	OutVM.TotalSelectedCount = 1;
	OutVM.CountLabel = TEXT("");
	OutVM.bShowProductionPanel = false;
	OutVM.Production = FBuildingProductionViewModel();
	OutVM.bShowDetailStats = false;
	OutVM.Damage = 0.f;
	OutVM.Armor = 0.f;
	OutVM.AttackSpeed = 0.f;
	OutVM.MoveSpeed = 0.f;
	OutVM.Range = 0.f;
	OutVM.CurrentHP = 0.f;
	OutVM.MaxHP = 0.f;
	OutVM.HPText = TEXT("");
	OutVM.SummaryItems.Empty();

	TWUIBridgeBuildingHelpers::AppendBuildModeCommandIds(CommandMetaTable, OutCommandIds);
	return true;
}

bool UTWPlayerUIBridge::TryBuildingSelectionVM(FSelectionViewModel& OutVM, TArray<FName>& OutCommandIds)
{
	if (!OwnerController)
	{
		return false;
	}

	ATWBaseBuilding* SelectedBuilding = OwnerController->GetSelectedBuilding();
	if (!SelectedBuilding)
	{
		return false;
	}

	const ATWPlayerState* LocalPS = OwnerController->GetPlayerState<ATWPlayerState>();
	const bool bIsOwnedByMe =
		LocalPS &&
		(SelectedBuilding->GetOwnerPlayerSlot() == LocalPS->PlayerSlot);

	FillCommonBuildingSelectionVM(SelectedBuilding, OutVM);
	OutCommandIds.Reset();

	if (!bIsOwnedByMe)
	{
		StopSelectionRefreshTimer();
		return true;
	}

	if (ATWTroopSpawnBuilding* TroopBuilding = Cast<ATWTroopSpawnBuilding>(SelectedBuilding))
	{
		FillTroopBuildingSelectionData(TroopBuilding, OutVM, OutCommandIds);
		return true;
	}

	if (ATWUpgradeBuilding* UpgradeBuilding = Cast<ATWUpgradeBuilding>(SelectedBuilding))
	{
		FillUpgradeBuildingSelectionData(UpgradeBuilding, OutVM, OutCommandIds);
		return true;
	}

	if (ATWPopulationBuilding* PopulationBuilding = Cast<ATWPopulationBuilding>(SelectedBuilding))
	{
		FillPopulationBuildingSelectionData(PopulationBuilding, OutVM, OutCommandIds);
		return true;
	}

	StopSelectionRefreshTimer();
	return true;
}

bool UTWPlayerUIBridge::TryUnitSelectionVM(FSelectionViewModel& OutVM, TArray<FName>& OutCommandIds)
{
	if (!OwnerController)
	{
		return false;
	}
	const bool bHasHeroSelected =
	OwnerController &&
	OwnerController->IsOwnedHeroCurrentlySelected();

	if (bHasHeroSelected)
	{
		StartSelectionRefreshTimer(0.0f);
	}
	else
	{
		StopSelectionRefreshTimer();
	}

	const int32 LocalSelectedUnitCount = OwnerController->GetLocalSelectedUnitCount();
	if (LocalSelectedUnitCount <= 0)
	{
		return false;
	}

	const FName LocalPrimarySelectedUnitId = OwnerController->GetLocalPrimarySelectedUnitId();
	const TArray<FSelectionSummaryItemViewModel>& LocalSummaryItems = OwnerController->GetLocalSelectionSummaryItems();

	const bool bIsMultiSelection = LocalSelectedUnitCount > 1;

	const ATWPlayerState* LocalPS = OwnerController->GetPlayerState<ATWPlayerState>();
	const bool bLocalUnitSelectionOwnedByMe =
		LocalPS &&
		(OwnerController->GetLocalSelectedOwnerPlayerSlot() == LocalPS->PlayerSlot);

	OutVM = FSelectionViewModel();
	OutVM.SelectionType = bIsMultiSelection ? ESelectionViewType::Multi : ESelectionViewType::Unit;
	OutVM.ViewMode = bIsMultiSelection ? ESelectionViewMode::Multi : ESelectionViewMode::Single;
	OutVM.SelectionId = LocalPrimarySelectedUnitId.IsNone()
		? (bIsMultiSelection ? DefaultMultiSelectedUnitId : DefaultSelectedUnitId)
		: LocalPrimarySelectedUnitId;

	OutVM.DisplayName = bIsMultiSelection ? TWUIBridgeText::MultiSelectedUnits : OutVM.SelectionId.ToString();
	OutVM.TypeLabel = bIsMultiSelection ? TWUIBridgeText::UnitsTypeLabel : TWUIBridgeText::UnitTypeLabel;
	OutVM.TotalSelectedCount = LocalSelectedUnitCount;
	OutVM.CountLabel = bIsMultiSelection
		? FString::Printf(TEXT("%d Selected"), LocalSelectedUnitCount)
		: TEXT("");
	OutVM.bShowProductionPanel = false;
	OutVM.Production = FBuildingProductionViewModel();
	OutVM.bShowDetailStats = false;
	OutVM.Damage = 0.f;
	OutVM.Armor = 0.f;
	OutVM.AttackSpeed = 0.f;
	OutVM.MoveSpeed = 0.f;
	OutVM.Range = 0.f;

	if (bIsMultiSelection)
	{
		OutVM.SummaryItems = LocalSummaryItems;
	}
	else
	{
		float CurrentHP = 0.f;
		float MaxHP = 0.f;
		bool bHasAnyHP = false;

		FTWUnitStatus BaseStatus;
		bool bHasBaseStatus = false;

		OutVM.DisplayName = OutVM.SelectionId.ToString();
		OutVM.TypeLabel = TWUIBridgeText::UnitTypeLabel;

		if (UTWUnitSubsystem* UnitSubsystem = OwnerController->GetWorld()->GetSubsystem<UTWUnitSubsystem>())
		{
			if (FTWUnitTableRowBase* UnitRow = UnitSubsystem->GetUnitTableRowBase(OutVM.SelectionId))
			{
				if (!UnitRow->UnitID.IsNone())
				{
					OutVM.DisplayName = UnitRow->UnitID.ToString();
				}
			}

			const int32 OwnerPlayerSlot = OwnerController->GetLocalSelectedOwnerPlayerSlot();
			BaseStatus = UnitSubsystem->GetUnitDefaultStatus(OutVM.SelectionId, OwnerPlayerSlot);
			bHasBaseStatus = true;

			MaxHP = BaseStatus.GetStatus(ETWStatusType::Health);
			if (MaxHP > 0.f)
			{
				bHasAnyHP = true;
			}
		}

		const FTWUnitStatus* RuntimeStatusPtr = nullptr;
		FTWUnitStatus RuntimeStatus;

		if (OwnerController->HasLocalPrimarySelectedUnitStatus())
		{
			RuntimeStatus = OwnerController->GetLocalPrimarySelectedUnitStatus();
			RuntimeStatusPtr = &RuntimeStatus;

			CurrentHP = RuntimeStatus.GetStatus(ETWStatusType::Health);
			if (CurrentHP > 0.f)
			{
				bHasAnyHP = true;
			}

			if (MaxHP <= 0.f && bHasBaseStatus)
			{
				MaxHP = BaseStatus.GetStatus(ETWStatusType::Health);
			}
			else if (MaxHP <= 0.f)
			{
				MaxHP = CurrentHP;
			}
		}

		OutVM.CurrentHP = CurrentHP;
		OutVM.MaxHP = MaxHP;
		OutVM.HPText = bHasAnyHP
			? FString::Printf(TEXT("%.0f / %.0f"), CurrentHP, MaxHP)
			: TEXT("");

		TWUIBridgeUnitHelpers::FillUnitDetailStats(
			OutVM,
			RuntimeStatusPtr,
			bHasBaseStatus ? &BaseStatus : nullptr
		);
	}

	OutCommandIds.Reset();
	if (bLocalUnitSelectionOwnedByMe)
	{
		OutCommandIds = {
			TWCommandIds::Move,
			TWCommandIds::Attack,
			TWCommandIds::Hold
		};

		if (OwnerController && OwnerController->IsOwnedHeroCurrentlySelected())
		{
			OutCommandIds.AddUnique(TWCommandIds::HeroSkill);
		}
	}

	return true;
}

void UTWPlayerUIBridge::ApplyCommandQueueCounts(const TArray<FName>& CommandIds)
{
	if (!SelectionProvider)
	{
		return;
	}

	SelectionProvider->ClearRuntimeCommandData();

	for (const FName& CommandId : CommandIds)
	{
		const int32 QueueCount = ResolveBuildingQueueCount(CommandId);
		SelectionProvider->SetRuntimeCommandQueueCount(CommandId, QueueCount);
	}
	if (OwnerController && OwnerController->IsOwnedHeroCurrentlySelected())
	{
		ATWHeroUnitBase* OwnedHero = OwnerController->GetOwnedHeroUnit();
		if (OwnedHero && SelectionProvider)
		{
			const float Remaining = OwnedHero->GetRemainingCooldownTime();
			const bool bReady = (Remaining <= KINDA_SMALL_NUMBER);

			SelectionProvider->SetRuntimeCommandEnabled(
				TWCommandIds::HeroSkill,
				bReady
			);

			SelectionProvider->SetRuntimeCommandDescription(
				TWCommandIds::HeroSkill,
				bReady ? TEXT("") : FString::Printf(TEXT("%.1fs"), Remaining)
			);
		}
	}
}

void UTWPlayerUIBridge::ApplySelectionResult(
	const FSelectionViewModel& VM,
	const TArray<FName>& CommandIds,
	ESelectionViewMode ViewMode,
	int32 TotalCount,
	const TArray<FSelectionSummaryItemViewModel>& SummaryItems
)
{
	if (!SelectionProvider)
	{
		return;
	}

	CurrentVisibleCommandIds = CommandIds;

	SelectionProvider->SetRuntimeSelection(
		VM,
		CommandIds,
		ViewMode,
		VM.SelectionId,
		TotalCount,
		SummaryItems
	);

	ApplyCommandQueueCounts(CommandIds);
}

void UTWPlayerUIBridge::ClearSelectionResult()
{
	if (!SelectionProvider)
	{
		return;
	}

	CurrentVisibleCommandIds.Empty();
	SelectionProvider->ClearRuntimeSelection();
	SelectionProvider->ClearRuntimeCommandData();
	StopSelectionRefreshTimer();
}

void UTWPlayerUIBridge::RefreshSelection()
{
	if (!SelectionProvider || !OwnerController)
	{
		return;
	}

	FSelectionViewModel VM;
	TArray<FName> CommandIds;

	if (TryBuildModeSelectionVM(VM, CommandIds))
	{
		ApplySelectionResult(
			VM,
			CommandIds,
			ESelectionViewMode::Single,
			1,
			TArray<FSelectionSummaryItemViewModel>()
		);
		return;
	}

	if (TryBuildingSelectionVM(VM, CommandIds))
	{
		ApplySelectionResult(
			VM,
			CommandIds,
			ESelectionViewMode::Single,
			1,
			TArray<FSelectionSummaryItemViewModel>()
		);
		return;
	}

	if (TryUnitSelectionVM(VM, CommandIds))
	{
		ApplySelectionResult(
			VM,
			CommandIds,
			VM.ViewMode,
			VM.TotalSelectedCount,
			VM.SummaryItems
		);
		return;
	}

	ClearSelectionResult();
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
	int32 WoodUpkeep = 0;
	int32 OreUpkeep = 0;

	const ATWPlayerState* TWPS = OwnerController->GetPlayerState<ATWPlayerState>();
	if (TWPS)
	{
		Wood = TWPS->GetResourceAmount(EResourceType::Wood);
		Gas = TWPS->GetResourceAmount(EResourceType::Ore);
		Mithril = TWPS->GetResourceAmount(EResourceType::Mithril);
		CurrentPopulation = TWPS->GetCurrentPopulation();
		PendingPopulation = TWPS->GetPendingPopulation();
		PopulationLimit = TWPS->GetPopulationLimit();
		MaxPopulation = TWPS->GetMaxPopulation();
		WoodUpkeep = TWPS->GetReplicatedWoodUpkeep();
		OreUpkeep = TWPS->GetReplicatedOreUpkeep();
	}

	const int32 DisplayPopulation = CurrentPopulation + PendingPopulation;

	VM.Wood = Wood;
	VM.Gas = Gas;
	VM.Mithril = Mithril;
	VM.Population = DisplayPopulation;
	VM.PopulationText = FString::Printf(TEXT("%d / %d"), DisplayPopulation, PopulationLimit);
	VM.WoodUpkeep = WoodUpkeep;
	VM.GasUpkeep = OreUpkeep;

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

void UTWPlayerUIBridge::RefreshBuildModeNotification()
{
	if (!OwnerController || !HUDRootWidget)
	{
		return;
	}

	const bool bIsBuildMode = OwnerController->IsBuildShortcutModeActive();
	HUDRootWidget->SetBuildModeNotification(bIsBuildMode);
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

	if (const ATWTroopSpawnBuilding* TroopBuilding = Cast<ATWTroopSpawnBuilding>(SelectedBuilding))
	{
		const FUICommandMetaRow* CommandMeta = FindCommandMetaRow(CommandId);
		if (!CommandMeta || CommandMeta->CommandType != ETWCommandType::ProduceUnit)
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

	if (const ATWUpgradeBuilding* UpgradeBuilding = Cast<ATWUpgradeBuilding>(SelectedBuilding))
	{
		const FUICommandMetaRow* CommandMeta = FindCommandMetaRow(CommandId);
		if (!CommandMeta || CommandMeta->CommandType != ETWCommandType::Research)
		{
			return 0;
		}

		int32 MatchedCount = 0;
		const TArray<FName>& QueuedUpgradeIds = UpgradeBuilding->GetQueuedUpgradeIds();

		for (const FName& QueuedUpgradeId : QueuedUpgradeIds)
		{
			if (QueuedUpgradeId == CommandMeta->PayloadId)
			{
				++MatchedCount;
			}
		}

		return MatchedCount;
	}

	if (const ATWPopulationBuilding* PopulationBuilding = Cast<ATWPopulationBuilding>(SelectedBuilding))
	{
		if (CommandId == TWCommandIds::IncreasePopulation)
		{
			return PopulationBuilding->GetCurrentQueueCount();
		}
	}

	return 0;
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

	if (bEdgeScrollingActive)
	{
		VM.CursorVisualType = ETWCursorVisualType::EdgeScroll;
	}
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