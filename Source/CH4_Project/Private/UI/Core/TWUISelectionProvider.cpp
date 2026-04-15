#include "UI/Core/TWUISelectionProvider.h"

#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Building/TWBaseBuilding.h"
#include "Subsystems/TWUnitSubsystem.h"

UTWUISelectionProvider::UTWUISelectionProvider()
{
	SeedDefaultCommandMap();
	SeedDefaultContextCommandMap();
	DebugClearSelection();
}

void UTWUISelectionProvider::Initialize(UObject* InSelectionSystemSource)
{
	SelectionSystemSource = InSelectionSystemSource;
}

void UTWUISelectionProvider::Shutdown()
{
	SelectionSystemSource = nullptr;
}

void UTWUISelectionProvider::SetUseDebugFallback(bool bInUseDebugFallback)
{
	bUseDebugFallback = bInUseDebugFallback;
}

void UTWUISelectionProvider::SetRuntimeSelection(
	const FSelectionViewModel& InSelectionViewModel,
	const TArray<FName>& InCommandIds,
	ESelectionViewMode InViewMode,
	FName InPrimaryEntityId,
	int32 InTotalSelectedCount,
	const TArray<FSelectionSummaryItemViewModel>& InSummaryItems
)
{
	CachedSelectionViewModel = InSelectionViewModel;
	CachedCommandIds = InCommandIds;
	CachedViewMode = InViewMode;
	CachedPrimaryEntityId = InPrimaryEntityId;
	CachedTotalSelectedCount = InTotalSelectedCount;
	CachedSummaryItems = InSummaryItems;

	CachedSelectionViewModel.ViewMode = CachedViewMode;
	CachedSelectionViewModel.TotalSelectedCount = CachedTotalSelectedCount;
	CachedSelectionViewModel.SummaryItems = CachedSummaryItems;

	if (CachedSelectionViewModel.SelectionId.IsNone())
	{
		CachedSelectionViewModel.SelectionId = CachedPrimaryEntityId;
	}

	ResetCommandContext();

	OnSelectionChanged.Broadcast();
	OnCommandContextChanged.Broadcast();
}

void UTWUISelectionProvider::ClearRuntimeSelection()
{
	DebugClearSelection();
}

void UTWUISelectionProvider::DebugSetSelection(
	const FSelectionViewModel& InSelectionViewModel,
	const TArray<FName>& InCommandIds,
	ESelectionViewMode InViewMode,
	FName InPrimaryEntityId,
	int32 InTotalSelectedCount,
	const TArray<FSelectionSummaryItemViewModel>& InSummaryItems
)
{
	CachedSelectionViewModel = InSelectionViewModel;
	CachedCommandIds = InCommandIds;
	CachedViewMode = InViewMode;
	CachedPrimaryEntityId = InPrimaryEntityId;
	CachedTotalSelectedCount = InTotalSelectedCount;
	CachedSummaryItems = InSummaryItems;

	CachedSelectionViewModel.ViewMode = CachedViewMode;
	CachedSelectionViewModel.TotalSelectedCount = CachedTotalSelectedCount;
	CachedSelectionViewModel.SummaryItems = CachedSummaryItems;

	if (CachedSelectionViewModel.SelectionId.IsNone())
	{
		CachedSelectionViewModel.SelectionId = CachedPrimaryEntityId;
	}

	ResetCommandContext();

	OnSelectionChanged.Broadcast();
	OnCommandContextChanged.Broadcast();
}

void UTWUISelectionProvider::DebugClearSelection()
{
	CachedSelectionViewModel = FSelectionViewModel();
	CachedCommandIds.Reset();
	CachedViewMode = ESelectionViewMode::None;
	CachedPrimaryEntityId = NAME_None;
	CachedTotalSelectedCount = 0;
	CachedSummaryItems.Reset();

	CachedSelectionViewModel.SelectionType = ESelectionViewType::None;
	CachedSelectionViewModel.ViewMode = ESelectionViewMode::None;
	CachedSelectionViewModel.DisplayName = TEXT("No Selection");
	CachedSelectionViewModel.TypeLabel = TEXT("");
	CachedSelectionViewModel.SelectionId = NAME_None;
	CachedSelectionViewModel.TotalSelectedCount = 0;
	CachedSelectionViewModel.CountLabel = TEXT("");
	CachedSelectionViewModel.SummaryItems.Reset();

	ResetCommandContext();

	OnSelectionChanged.Broadcast();
	OnCommandContextChanged.Broadcast();
}

FSelectionViewModel UTWUISelectionProvider::GetSelectionViewModel_Implementation() const
{
	RefreshFromSourceIfNeeded();
	return CachedSelectionViewModel;
}

TArray<FName> UTWUISelectionProvider::GetCurrentCommandIds_Implementation() const
{
	RefreshFromSourceIfNeeded();
	return CachedCommandIds;
}

ESelectionViewMode UTWUISelectionProvider::GetSelectionViewMode() const
{
	return CachedViewMode;
}

FName UTWUISelectionProvider::GetPrimaryEntityId() const
{
	return CachedPrimaryEntityId;
}

int32 UTWUISelectionProvider::GetTotalSelectedCount() const
{
	return CachedTotalSelectedCount;
}

TArray<FSelectionSummaryItemViewModel> UTWUISelectionProvider::GetSelectionSummary() const
{
	return CachedSummaryItems;
}

TArray<FName> UTWUISelectionProvider::GetCommandIdsForSelectionEntity(const FName& InEntityId) const
{
	if (const TArray<FName>* Found = EntityCommandMap.Find(InEntityId))
	{
		return *Found;
	}

	return {};
}

FName UTWUISelectionProvider::GetCurrentCommandContext() const
{
	return CurrentCommandContext;
}

TArray<FName> UTWUISelectionProvider::GetCommandIdsForCurrentContext() const
{
	if (CurrentCommandContext.IsNone())
	{
		return CachedCommandIds;
	}

	if (const TArray<FName>* Found = ContextCommandMap.Find(CurrentCommandContext))
	{
		return *Found;
	}

	return CachedCommandIds;
}

void UTWUISelectionProvider::HandleCommandInput(const FName& InCommandId, const FUICommandMetaRow* InCommandMetaRow)
{
	if (InCommandId.IsNone())
	{
		return;
	}

	if (InCommandMetaRow)
	{
		if (InCommandMetaRow->bReturnToPreviousContext)
		{
			PopCommandContext();
			return;
		}

		if (InCommandMetaRow->bIsContextCommand && !InCommandMetaRow->NextContext.IsNone())
		{
			PushAndSetCommandContext(InCommandMetaRow->NextContext);
			return;
		}
	}

	if (InCommandId == TEXT("Back"))
	{
		PopCommandContext();
		return;
	}

	if (InCommandId == TEXT("BuildMenu"))
	{
		PushAndSetCommandContext(TEXT("Build"));
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[TWUISelectionProvider] Execute Command: %s (Context=%s)"),
		*InCommandId.ToString(),
		*CurrentCommandContext.ToString()
	);
}

FOnUICommandContextChanged& UTWUISelectionProvider::GetOnCommandContextChangedDelegate()
{
	return OnCommandContextChanged;
}

FOnUISelectionChanged& UTWUISelectionProvider::GetOnSelectionChangedDelegate()
{
	return OnSelectionChanged;
}

void UTWUISelectionProvider::SeedDefaultCommandMap()
{
	EntityCommandMap.Reset();

	EntityCommandMap.Add(TEXT("Engineer"), { TEXT("Move"), TEXT("Attack"), TEXT("Hold"), TEXT("BuildMenu") });
	EntityCommandMap.Add(TEXT("Barracks"), { TEXT("TrainFootman"), TEXT("TrainArcher") });
	EntityCommandMap.Add(TEXT("Footman"), { TEXT("Move"), TEXT("Attack"), TEXT("Hold") });
	EntityCommandMap.Add(TEXT("Archer"), { TEXT("Move"), TEXT("Attack"), TEXT("Hold") });
}

void UTWUISelectionProvider::SeedDefaultContextCommandMap()
{
	ContextCommandMap.Reset();

	ContextCommandMap.Add(
		TEXT("Build"),
		{
			TEXT("BuildBarracks"),
			TEXT("BuildArcheryRange"),
			TEXT("Back")
		}
	);
}

void UTWUISelectionProvider::ResetCommandContext()
{
	CommandContextStack.Reset();
	CurrentCommandContext = NAME_None;
}

void UTWUISelectionProvider::PushAndSetCommandContext(FName InNewContext)
{
	CommandContextStack.Add(CurrentCommandContext);
	CurrentCommandContext = InNewContext;
	OnCommandContextChanged.Broadcast();
}

void UTWUISelectionProvider::PopCommandContext()
{
	if (CommandContextStack.Num() > 0)
	{
		CurrentCommandContext = CommandContextStack.Pop();
	}
	else
	{
		CurrentCommandContext = NAME_None;
	}

	OnCommandContextChanged.Broadcast();
}

void UTWUISelectionProvider::RefreshFromSourceIfNeeded() const
{
	if (bUseDebugFallback)
	{
		return;
	}

	const ATWPlayerController* TWPC = Cast<ATWPlayerController>(SelectionSystemSource);
	if (!TWPC)
	{
		return;
	}

	// 브리지에서 SetRuntimeSelection으로 밀어준 상태가 이미 있으면 그대로 사용
	if (CachedSelectionViewModel.SelectionType != ESelectionViewType::None)
	{
		return;
	}

	// fallback: 단일 건물 선택만 간단 반영
	if (ATWBaseBuilding* SelectedBuilding = TWPC->GetPrimarySelectedBuilding())
	{
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel = FSelectionViewModel();
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SelectionType = ESelectionViewType::Building;
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.ViewMode = ESelectionViewMode::Single;
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SelectionId = TWPC->ResolveBuildingSelectionId(SelectedBuilding);
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.DisplayName = SelectedBuilding->GetName();
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.TypeLabel = TEXT("Building");
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.TotalSelectedCount = TWPC->GetLocalSelectedBuildingCount();

		const_cast<UTWUISelectionProvider*>(this)->CachedViewMode = ESelectionViewMode::Single;
		const_cast<UTWUISelectionProvider*>(this)->CachedPrimaryEntityId =
			const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SelectionId;
		const_cast<UTWUISelectionProvider*>(this)->CachedTotalSelectedCount =
			const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.TotalSelectedCount;
		return;
	}

	// fallback: 유닛 선택
	if (TWPC->GetLocalSelectedUnitCount() > 0)
	{
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel = FSelectionViewModel();
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SelectionType =
			(TWPC->GetLocalSelectedUnitCount() > 1) ? ESelectionViewType::Multi : ESelectionViewType::Unit;
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.ViewMode =
			(TWPC->GetLocalSelectedUnitCount() > 1) ? ESelectionViewMode::Multi : ESelectionViewMode::Single;
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SelectionId =
			TWPC->GetLocalPrimarySelectedUnitId();
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.TotalSelectedCount =
			TWPC->GetLocalSelectedUnitCount();
		const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SummaryItems =
			TWPC->GetLocalSelectionSummaryItems();

		const_cast<UTWUISelectionProvider*>(this)->CachedViewMode =
			const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.ViewMode;
		const_cast<UTWUISelectionProvider*>(this)->CachedPrimaryEntityId =
			const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SelectionId;
		const_cast<UTWUISelectionProvider*>(this)->CachedTotalSelectedCount =
			const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.TotalSelectedCount;
		const_cast<UTWUISelectionProvider*>(this)->CachedSummaryItems =
			const_cast<UTWUISelectionProvider*>(this)->CachedSelectionViewModel.SummaryItems;
	}
}

void UTWUISelectionProvider::ClearRuntimeCommandData()
{
	RuntimeCommandQueueCounts.Empty();
}

void UTWUISelectionProvider::SetRuntimeCommandQueueCount(FName CommandId, int32 QueueCount)
{
	if (CommandId.IsNone())
	{
		return;
	}

	if (QueueCount <= 0)
	{
		RuntimeCommandQueueCounts.Remove(CommandId);
		return;
	}

	RuntimeCommandQueueCounts.Add(CommandId, QueueCount);
}

int32 UTWUISelectionProvider::GetRuntimeCommandQueueCount(FName CommandId) const
{
	if (const int32* Found = RuntimeCommandQueueCounts.Find(CommandId))
	{
		return *Found;
	}

	return 0;
}