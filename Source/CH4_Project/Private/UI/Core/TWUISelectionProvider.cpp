#include "UI/Core/TWUISelectionProvider.h"

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
	if (!bUseDebugFallback)
	{
		// TODO: 실제 SelectionSystem 연동은 다음 단계에서 구현
	}
}