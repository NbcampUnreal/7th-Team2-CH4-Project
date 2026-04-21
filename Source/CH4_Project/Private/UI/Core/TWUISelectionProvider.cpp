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
	const bool bSelectionIdentityChanged =
		(CachedSelectionViewModel.SelectionType != InSelectionViewModel.SelectionType) ||
		(CachedPrimaryEntityId != InPrimaryEntityId) ||
		(CachedViewMode != InViewMode);

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
	
	if (bSelectionIdentityChanged)
	{
		ResetCommandContext();
		OnCommandContextChanged.Broadcast();
	}

	OnSelectionChanged.Broadcast();
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
	CachedSelectionViewModel.bShowDetailStats = false;
	CachedSelectionViewModel.Damage = 0.f;
	CachedSelectionViewModel.Armor = 0.f;
	CachedSelectionViewModel.AttackSpeed = 0.f;
	CachedSelectionViewModel.MoveSpeed = 0.f;
	CachedSelectionViewModel.Range = 0.f;

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
	EntityCommandMap.Add(TEXT("DragonKnight"), { TEXT("Move"), TEXT("Attack"), TEXT("Hold"), TEXT("HeroSkill") });
	EntityCommandMap.Add(TEXT("Markman"), { TEXT("Move"), TEXT("Attack"), TEXT("Hold"), TEXT("HeroSkill") });
	EntityCommandMap.Add(TEXT("Astrologian"), { TEXT("Move"), TEXT("Attack"), TEXT("Hold"), TEXT("HeroSkill") });
}

void UTWUISelectionProvider::SeedDefaultContextCommandMap()
{
	ContextCommandMap.Reset();
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
	
	if (CachedSelectionViewModel.SelectionType != ESelectionViewType::None)
	{
		return;
	}
	
	if (ATWBaseBuilding* SelectedBuilding = TWPC->GetSelectedBuilding())
	{
		UTWUISelectionProvider* MutableThis = const_cast<UTWUISelectionProvider*>(this);

		MutableThis->CachedSelectionViewModel = FSelectionViewModel();
		MutableThis->CachedSelectionViewModel.SelectionType = ESelectionViewType::Building;
		MutableThis->CachedSelectionViewModel.ViewMode = ESelectionViewMode::Single;
		MutableThis->CachedSelectionViewModel.SelectionId = TWPC->ResolveBuildingSelectionId(SelectedBuilding);
		MutableThis->CachedSelectionViewModel.DisplayName = SelectedBuilding->GetName();
		MutableThis->CachedSelectionViewModel.TypeLabel = TEXT("Building");
		MutableThis->CachedSelectionViewModel.TotalSelectedCount = 1;
		MutableThis->CachedSelectionViewModel.SummaryItems.Reset();
		MutableThis->CachedSelectionViewModel.bShowDetailStats = false;

		MutableThis->CachedViewMode = ESelectionViewMode::Single;
		MutableThis->CachedPrimaryEntityId = MutableThis->CachedSelectionViewModel.SelectionId;
		MutableThis->CachedTotalSelectedCount = 1;
		MutableThis->CachedSummaryItems.Reset();
		return;
	}
	
	if (TWPC->GetLocalSelectedUnitCount() > 0)
	{
		UTWUISelectionProvider* MutableThis = const_cast<UTWUISelectionProvider*>(this);

		MutableThis->CachedSelectionViewModel = FSelectionViewModel();
		MutableThis->CachedSelectionViewModel.SelectionType =
			(TWPC->GetLocalSelectedUnitCount() > 1) ? ESelectionViewType::Multi : ESelectionViewType::Unit;
		MutableThis->CachedSelectionViewModel.ViewMode =
			(TWPC->GetLocalSelectedUnitCount() > 1) ? ESelectionViewMode::Multi : ESelectionViewMode::Single;
		MutableThis->CachedSelectionViewModel.SelectionId = TWPC->GetLocalPrimarySelectedUnitId();
		MutableThis->CachedSelectionViewModel.TotalSelectedCount = TWPC->GetLocalSelectedUnitCount();
		MutableThis->CachedSelectionViewModel.SummaryItems = TWPC->GetLocalSelectionSummaryItems();
		MutableThis->CachedSelectionViewModel.bShowDetailStats =
			(TWPC->GetLocalSelectedUnitCount() == 1);

		MutableThis->CachedViewMode = MutableThis->CachedSelectionViewModel.ViewMode;
		MutableThis->CachedPrimaryEntityId = MutableThis->CachedSelectionViewModel.SelectionId;
		MutableThis->CachedTotalSelectedCount = MutableThis->CachedSelectionViewModel.TotalSelectedCount;
		MutableThis->CachedSummaryItems = MutableThis->CachedSelectionViewModel.SummaryItems;
	}
}

void UTWUISelectionProvider::ClearRuntimeCommandData()
{
	RuntimeCommandQueueCounts.Empty();
	RuntimeCommandEnabledMap.Empty();
	RuntimeCommandDescriptionMap.Empty();
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

void UTWUISelectionProvider::SetRuntimeCommandEnabled(FName CommandId, bool bEnabled)
{
	if (CommandId.IsNone())
	{
		return;
	}

	RuntimeCommandEnabledMap.Add(CommandId, bEnabled);
}

bool UTWUISelectionProvider::GetRuntimeCommandEnabled(FName CommandId, bool& OutEnabled) const
{
	if (const bool* Found = RuntimeCommandEnabledMap.Find(CommandId))
	{
		OutEnabled = *Found;
		return true;
	}

	return false;
}

void UTWUISelectionProvider::SetRuntimeCommandDescription(FName CommandId, const FString& InDescription)
{
	if (CommandId.IsNone())
	{
		return;
	}

	if (InDescription.IsEmpty())
	{
		RuntimeCommandDescriptionMap.Remove(CommandId);
		return;
	}

	RuntimeCommandDescriptionMap.Add(CommandId, InDescription);
}

bool UTWUISelectionProvider::GetRuntimeCommandDescription(FName CommandId, FString& OutDescription) const
{
	if (const FString* Found = RuntimeCommandDescriptionMap.Find(CommandId))
	{
		OutDescription = *Found;
		return true;
	}

	return false;
}