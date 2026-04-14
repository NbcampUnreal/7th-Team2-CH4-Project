#include "UI/Core/TWUIHUDCoordinator.h"

#include "Engine/DataTable.h"
#include "UI/Core/TWResourceDataProvider.h"
#include "UI/Core/TWSelectionDataProvider.h"
#include "UI/Widgets/TWHUDRootWidget.h"

void UTWUIHUDCoordinator::Initialize(
	UTWHUDRootWidget* InHUDRoot,
	UObject* InSelectionProvider,
	UObject* InResourceProvider,
	UDataTable* InCommandMetaTable,
	UDataTable* InSelectionPresentationTable)
{
	UnbindProviderDelegates();
	UnbindHUDDelegates();

	HUDRoot = InHUDRoot;
	SelectionProvider = InSelectionProvider;
	ResourceProvider = InResourceProvider;
	CommandMetaTable = InCommandMetaTable;
	SelectionPresentationTable = InSelectionPresentationTable;
	LastBuiltCommandViewModels.Reset();

	BindProviderDelegates();
	BindHUDDelegates();

	RefreshAll();
	RefreshMenuPanel();
	RefreshMinimapPanel();
}

void UTWUIHUDCoordinator::Shutdown()
{
	UnbindProviderDelegates();
	UnbindHUDDelegates();

	HUDRoot = nullptr;
	SelectionProvider = nullptr;
	ResourceProvider = nullptr;
	CommandMetaTable = nullptr;
	SelectionPresentationTable = nullptr;
	LastBuiltCommandViewModels.Reset();
}

void UTWUIHUDCoordinator::BindProviderDelegates()
{
	if (ITWSelectionDataProvider* SelectionProviderInterface = GetSelectionProviderInterface())
	{
		SelectionProviderInterface->GetOnSelectionChangedDelegate().RemoveAll(this);
		SelectionProviderInterface->GetOnSelectionChangedDelegate().AddUObject(
			this,
			&UTWUIHUDCoordinator::HandleSelectionChanged);

		SelectionProviderInterface->GetOnCommandContextChangedDelegate().RemoveAll(this);
		SelectionProviderInterface->GetOnCommandContextChangedDelegate().AddUObject(
			this,
			&UTWUIHUDCoordinator::HandleCommandContextChanged);
	}

	if (ITWResourceDataProvider* ResourceProviderInterface = GetResourceProviderInterface())
	{
		ResourceProviderInterface->GetOnResourceChangedDelegate().RemoveAll(this);
		ResourceProviderInterface->GetOnResourceChangedDelegate().AddUObject(
			this,
			&UTWUIHUDCoordinator::HandleResourceChanged);
	}
}

void UTWUIHUDCoordinator::UnbindProviderDelegates()
{
	if (ITWSelectionDataProvider* SelectionProviderInterface = GetSelectionProviderInterface())
	{
		SelectionProviderInterface->GetOnSelectionChangedDelegate().RemoveAll(this);
		SelectionProviderInterface->GetOnCommandContextChangedDelegate().RemoveAll(this);
	}

	if (ITWResourceDataProvider* ResourceProviderInterface = GetResourceProviderInterface())
	{
		ResourceProviderInterface->GetOnResourceChangedDelegate().RemoveAll(this);
	}
}

void UTWUIHUDCoordinator::BindHUDDelegates()
{
	if (!HUDRoot)
	{
		return;
	}

	HUDRoot->GetOnHUDCommandClickedDelegate().RemoveAll(this);
	HUDRoot->GetOnHUDCommandClickedDelegate().AddUObject(
		this,
		&UTWUIHUDCoordinator::HandleHUDCommandClicked
	);

	UE_LOG(LogTemp, Warning, TEXT("[HUDCoordinator] Bound HUD command click delegate"));
}

void UTWUIHUDCoordinator::UnbindHUDDelegates()
{
	if (!HUDRoot)
	{
		return;
	}

	HUDRoot->GetOnHUDCommandClickedDelegate().RemoveAll(this);
}

void UTWUIHUDCoordinator::HandleHUDCommandClicked(FName InCommandId)
{
	UE_LOG(LogTemp, Warning, TEXT("[HUDCoordinator] HandleHUDCommandClicked: %s"), *InCommandId.ToString());

	if (InCommandId.IsNone())
	{
		return;
	}

	OnCommandRequested.Broadcast(InCommandId);
}

void UTWUIHUDCoordinator::HandleSelectionChanged()
{
	RefreshSelectionPanel();
	RefreshCommandPanel();
}

void UTWUIHUDCoordinator::HandleResourceChanged()
{
	RefreshTopBar();
}

void UTWUIHUDCoordinator::HandleCommandContextChanged()
{
	RefreshCommandPanel();
}

void UTWUIHUDCoordinator::RefreshAll()
{
	RefreshTopBar();
	RefreshSelectionPanel();
	RefreshCommandPanel();
}

FTopBarViewModel UTWUIHUDCoordinator::BuildTopBarViewModel() const
{
	FTopBarViewModel VM;

	if (!HasValidResourceProvider())
	{
		return VM;
	}

	VM = ITWResourceDataProvider::Execute_GetTopBarViewModel(ResourceProvider);

	if (const ITWResourceDataProvider* ResourceProviderInterface = GetResourceProviderInterface())
	{
		VM.GameTimeText = FormatGameTime(ResourceProviderInterface->GetElapsedSeconds());
	}

	return VM;
}

FSelectionViewModel UTWUIHUDCoordinator::BuildSelectionViewModel() const
{
	FSelectionViewModel VM;

	if (!HasValidSelectionProvider())
	{
		return VM;
	}

	VM = ITWSelectionDataProvider::Execute_GetSelectionViewModel(SelectionProvider);

	if (const ITWSelectionDataProvider* SelectionProviderInterface = GetSelectionProviderInterface())
	{
		VM.ViewMode = SelectionProviderInterface->GetSelectionViewMode();

		if (VM.SelectionId.IsNone())
		{
			VM.SelectionId = SelectionProviderInterface->GetPrimaryEntityId();
		}

		VM.TotalSelectedCount = SelectionProviderInterface->GetTotalSelectedCount();
		VM.SummaryItems = SelectionProviderInterface->GetSelectionSummary();
	}

	if (VM.SelectionType == ESelectionViewType::None)
	{
		VM.ViewMode = ESelectionViewMode::None;
		VM.TotalSelectedCount = 0;
		VM.CountLabel = TEXT("");
	}
	else if (VM.SelectionType == ESelectionViewType::Multi)
	{
		VM.ViewMode = ESelectionViewMode::Multi;

		if (VM.TotalSelectedCount <= 0)
		{
			int32 TotalCount = 0;
			for (const FSelectionSummaryItemViewModel& Item : VM.SummaryItems)
			{
				TotalCount += FMath::Max(0, Item.Count);
			}
			VM.TotalSelectedCount = TotalCount;
		}

		if (VM.TotalSelectedCount > 0)
		{
			VM.CountLabel = FString::Printf(TEXT("%d Selected"), VM.TotalSelectedCount);
		}

		for (FSelectionSummaryItemViewModel& Item : VM.SummaryItems)
		{
			HydrateSummaryItemFromPresentation(Item);

			if (Item.CountText.IsEmpty() && Item.Count > 0)
			{
				Item.CountText = FString::Printf(TEXT("x%d"), Item.Count);
			}
		}
	}
	else
	{
		if (VM.ViewMode == ESelectionViewMode::None)
		{
			VM.ViewMode = ESelectionViewMode::Single;
		}

		if (VM.TotalSelectedCount <= 0)
		{
			VM.TotalSelectedCount = 1;
		}
	}

	if (const FUISelectionPresentationRow* Row = FindSelectionPresentationRow(VM.SelectionId))
	{
		if (VM.DisplayName.IsEmpty() || VM.DisplayName.Equals(TEXT("No Selection")))
		{
			VM.DisplayName = Row->DisplayName.ToString();
		}

		if (VM.TypeLabel.IsEmpty())
		{
			VM.TypeLabel = Row->TypeLabel.ToString();
		}

		if (VM.PortraitIcon.IsNull() && !Row->PortraitIcon.IsNull())
		{
			VM.PortraitIcon = Row->PortraitIcon;
		}

		if (VM.Portrait.IsNull() && !Row->Portrait.IsNull())
		{
			VM.Portrait = Row->Portrait;
		}
	}

	if (VM.PortraitIcon.IsNull() && !VM.Portrait.IsNull())
	{
		VM.PortraitIcon = VM.Portrait;
	}

	return VM;
}

TArray<FCommandSlotViewModel> UTWUIHUDCoordinator::BuildCommandViewModels() const
{
	TArray<FCommandSlotViewModel> VMArray;

	if (!HasValidSelectionProvider())
	{
		return VMArray;
	}

	const TArray<FName> CommandIds = BuildCommonCommandIds();

	for (int32 Index = 0; Index < CommandIds.Num(); ++Index)
	{
		VMArray.Add(BuildCommandSlotViewModel(CommandIds[Index], Index));
	}

	return VMArray;
}

FCommandSlotViewModel UTWUIHUDCoordinator::BuildCommandSlotViewModel(
	const FName& CommandId,
	int32 FallbackSlotIndex) const
{
	FCommandSlotViewModel VM;
	VM.CommandId = CommandId;
	VM.SlotIndex = FallbackSlotIndex;
	VM.DisplayName = CommandId.ToString();
	VM.HotkeyLabel = FString::FromInt((FallbackSlotIndex + 1) % 10);
	VM.Description = TEXT("");
	VM.bVisible = true;
	VM.bEnabled = true;
	VM.bPending = false;
	VM.CooldownRatio = 0.f;
	VM.QueueCount = 0;
	VM.DisableReason = TEXT("");
	VM.bIsContextCommand = false;
	VM.bReturnToPreviousContext = false;
	VM.NextContext = NAME_None;
	VM.bArmed = (CommandId == CurrentArmedCommandId);
	if (CommandMetaTable)
	{
		static const FString Context = TEXT("CommandMetaLookup");
		const FUICommandMetaRow* Row = CommandMetaTable->FindRow<FUICommandMetaRow>(CommandId, Context);

		if (Row)
		{
			VM.SlotIndex = (Row->SlotIndex >= 0) ? Row->SlotIndex : FallbackSlotIndex;

			if (!Row->DisplayName.IsEmpty())
			{
				VM.DisplayName = Row->DisplayName.ToString();
			}

			if (!Row->HotkeyLabel.IsEmpty())
			{
				VM.HotkeyLabel = Row->HotkeyLabel.ToString();
			}

			if (!Row->Description.IsEmpty())
			{
				VM.Description = Row->Description.ToString();
			}

			VM.Icon = Row->Icon;
			VM.NextContext = Row->NextContext;
			VM.bIsContextCommand = Row->bIsContextCommand;
			VM.bReturnToPreviousContext = Row->bReturnToPreviousContext;
		}
	}

	return VM;
}

TArray<FName> UTWUIHUDCoordinator::BuildCommonCommandIds() const
{
	TArray<FName> Fallback;

	if (!HasValidSelectionProvider())
	{
		return Fallback;
	}

	const ITWSelectionDataProvider* SelectionProviderInterface = GetSelectionProviderInterface();
	if (!SelectionProviderInterface)
	{
		return Fallback;
	}

	const TArray<FName> ContextCommands = SelectionProviderInterface->GetCommandIdsForCurrentContext();
	if (!SelectionProviderInterface->GetCurrentCommandContext().IsNone())
	{
		return ContextCommands;
	}

	Fallback = ITWSelectionDataProvider::Execute_GetCurrentCommandIds(SelectionProvider);

	if (SelectionProviderInterface->GetSelectionViewMode() != ESelectionViewMode::Multi)
	{
		return Fallback;
	}

	const TArray<FSelectionSummaryItemViewModel> SummaryItems = SelectionProviderInterface->GetSelectionSummary();
	if (SummaryItems.Num() == 0)
	{
		return Fallback;
	}

	TArray<FName> UniqueEntityIds;
	TSet<FName> SeenEntityIds;

	for (const FSelectionSummaryItemViewModel& Item : SummaryItems)
	{
		if (Item.EntityId.IsNone())
		{
			continue;
		}

		if (SeenEntityIds.Contains(Item.EntityId))
		{
			continue;
		}

		SeenEntityIds.Add(Item.EntityId);
		UniqueEntityIds.Add(Item.EntityId);
	}

	if (UniqueEntityIds.Num() == 0)
	{
		return Fallback;
	}

	TArray<FName> Intersection = SelectionProviderInterface->GetCommandIdsForSelectionEntity(UniqueEntityIds[0]);
	if (Intersection.Num() == 0)
	{
		return Fallback;
	}

	for (int32 Index = 1; Index < UniqueEntityIds.Num(); ++Index)
	{
		const TArray<FName> CurrentCommands =
			SelectionProviderInterface->GetCommandIdsForSelectionEntity(UniqueEntityIds[Index]);

		if (CurrentCommands.Num() == 0)
		{
			return Fallback;
		}

		const TSet<FName> CurrentCommandSet(CurrentCommands);

		Intersection.RemoveAll(
			[&CurrentCommandSet](const FName& InCommandId)
			{
				return !CurrentCommandSet.Contains(InCommandId);
			});
	}

	return Intersection;
}

FString UTWUIHUDCoordinator::FormatGameTime(int32 InElapsedSeconds) const
{
	const int32 SafeSeconds = FMath::Max(0, InElapsedSeconds);
	const int32 Minutes = SafeSeconds / 60;
	const int32 Seconds = SafeSeconds % 60;

	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

const FUICommandMetaRow* UTWUIHUDCoordinator::FindCommandMetaRow(const FName& InCommandId) const
{
	if (!CommandMetaTable || InCommandId.IsNone())
	{
		return nullptr;
	}

	static const FString Context = TEXT("CommandMetaLookup_Find");
	return CommandMetaTable->FindRow<FUICommandMetaRow>(InCommandId, Context);
}

const FUISelectionPresentationRow* UTWUIHUDCoordinator::FindSelectionPresentationRow(const FName& InSelectionId) const
{
	if (!SelectionPresentationTable || InSelectionId.IsNone())
	{
		return nullptr;
	}

	static const FString Context = TEXT("SelectionPresentationLookup");
	if (const FUISelectionPresentationRow* Row =
		SelectionPresentationTable->FindRow<FUISelectionPresentationRow>(InSelectionId, Context))
	{
		return Row;
	}

	static const FString ContextAll = TEXT("SelectionPresentationLookup_AllRows");
	TArray<FUISelectionPresentationRow*> Rows;
	SelectionPresentationTable->GetAllRows<FUISelectionPresentationRow>(ContextAll, Rows);

	for (const FUISelectionPresentationRow* Candidate : Rows)
	{
		if (Candidate && Candidate->EntityId == InSelectionId)
		{
			return Candidate;
		}
	}

	return nullptr;
}

void UTWUIHUDCoordinator::HydrateSummaryItemFromPresentation(FSelectionSummaryItemViewModel& InOutItem) const
{
	if (const FUISelectionPresentationRow* Row = FindSelectionPresentationRow(InOutItem.EntityId))
	{
		if (InOutItem.DisplayName.IsEmpty())
		{
			InOutItem.DisplayName = Row->DisplayName.ToString();
		}

		if (InOutItem.Icon.IsNull())
		{
			if (!Row->PortraitIcon.IsNull())
			{
				InOutItem.Icon = Row->PortraitIcon;
			}
			else if (!Row->Portrait.IsNull())
			{
				InOutItem.Icon = Row->Portrait;
			}
		}
	}
}

void UTWUIHUDCoordinator::RefreshTopBar()
{
	if (!HUDRoot)
	{
		return;
	}

	HUDRoot->SetTopBarData(BuildTopBarViewModel());
}

void UTWUIHUDCoordinator::RefreshSelectionPanel()
{
	if (!HUDRoot)
	{
		return;
	}

	HUDRoot->SetSelectionData(BuildSelectionViewModel());
}

void UTWUIHUDCoordinator::RefreshCommandPanel()
{
	if (!HUDRoot)
	{
		return;
	}

	LastBuiltCommandViewModels = BuildCommandViewModels();
	HUDRoot->SetCommandData(LastBuiltCommandViewModels);
}

void UTWUIHUDCoordinator::RefreshMenuPanel()
{
	if (!HUDRoot)
	{
		return;
	}

	TArray<FMenuButtonViewModel> VMArray;

	{
		FMenuButtonViewModel VM;
		VM.SlotIndex = 0;
		VM.ActionId = TEXT("Menu");
		VM.DisplayName = TEXT("MENU");
		VM.HotkeyLabel = TEXT("ESC");
		VM.bVisible = true;
		VM.bEnabled = true;
		VM.TooltipText = TEXT("Open game menu");
		VMArray.Add(VM);
	}

	{
		FMenuButtonViewModel VM;
		VM.SlotIndex = 1;
		VM.ActionId = TEXT("Objectives");
		VM.DisplayName = TEXT("OBJECTIVES");
		VM.HotkeyLabel = TEXT("O");
		VM.bVisible = true;
		VM.bEnabled = true;
		VM.TooltipText = TEXT("Show mission objectives");
		VMArray.Add(VM);
	}

	{
		FMenuButtonViewModel VM;
		VM.SlotIndex = 2;
		VM.ActionId = TEXT("Settings");
		VM.DisplayName = TEXT("SETTINGS");
		VM.HotkeyLabel = TEXT("F10");
		VM.bVisible = true;
		VM.bEnabled = true;
		VM.TooltipText = TEXT("Open settings");
		VMArray.Add(VM);
	}

	HUDRoot->SetMenuData(VMArray);
}

void UTWUIHUDCoordinator::RefreshMinimapPanel()
{
	if (!HUDRoot)
	{
		return;
	}

	FMinimapPanelViewModel VM;
	VM.Title = TEXT("MINIMAP");
	VM.PlaceholderText = TEXT("SceneCapture / RenderTarget / Marker Layer");
	VM.bVisible = true;
	VM.bInteractive = false;
	VM.bShowFrame = true;

	HUDRoot->SetMinimapData(VM);
}

void UTWUIHUDCoordinator::PushNotification(const FString& Message, ENotificationType Type)
{
	if (!HUDRoot)
	{
		return;
	}

	HUDRoot->ShowNotification(Message, Type);
}

bool UTWUIHUDCoordinator::HasValidSelectionProvider() const
{
	return SelectionProvider
		&& SelectionProvider->GetClass()->ImplementsInterface(UTWSelectionDataProvider::StaticClass());
}

bool UTWUIHUDCoordinator::HasValidResourceProvider() const
{
	return ResourceProvider
		&& ResourceProvider->GetClass()->ImplementsInterface(UTWResourceDataProvider::StaticClass());
}

ITWSelectionDataProvider* UTWUIHUDCoordinator::GetSelectionProviderInterface() const
{
	if (!HasValidSelectionProvider())
	{
		return nullptr;
	}

	return Cast<ITWSelectionDataProvider>(SelectionProvider);
}

ITWResourceDataProvider* UTWUIHUDCoordinator::GetResourceProviderInterface() const
{
	if (!HasValidResourceProvider())
	{
		return nullptr;
	}

	return Cast<ITWResourceDataProvider>(ResourceProvider);
}

void UTWUIHUDCoordinator::SetArmedCommandId(FName InCommandId)
{
	CurrentArmedCommandId = InCommandId;
	RefreshCommandPanel();
}