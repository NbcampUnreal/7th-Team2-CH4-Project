#include "Player/TWPlayerController.h"

#include "Components/InputComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

#include "Gameplay/Test/TWTestBuildingActor.h"
#include "Gameplay/Test/TWTestUnitActor.h"
#include "UI/Core/TWUIHUDCoordinator.h"
#include "UI/Core/TWUIResourceProvider.h"
#include "UI/Core/TWUISelectionProvider.h"
#include "UI/Data/TWUIDataTypes.h"
#include "UI/Widgets/TWHUDRootWidget.h"

ATWPlayerController::ATWPlayerController()
{
	bShowMouseCursor = true;
	bReplicates = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ATWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	CreateAndShowHUD();
	CreateProviders();
	CreateCoordinator();
	BindCoordinator();

	if (ResourceProvider)
	{
		FTopBarViewModel TopBarVM;
		TopBarVM.Wood = 500;
		TopBarVM.Gas = 250;
		TopBarVM.Population = 12;

		ResourceProvider->SetUseDebugFallback(true);
		ResourceProvider->DebugSetResources(TopBarVM, 0);
	}

	ClearSelection();
}

void ATWPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDRootWidget)
	{
		HUDRootWidget->GetOnHUDCommandClickedDelegate().RemoveAll(this);
	}

	if (HUDCoordinator)
	{
		HUDCoordinator->Shutdown();
		HUDCoordinator = nullptr;
	}

	ShutdownProviders();
	HUDRootWidget = nullptr;
	CurrentSelectedActor = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ATWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TWPlayerController] InputComponent is null."));
		return;
	}

	InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ThisClass::HandleLeftClick);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ThisClass::HandleRightClick);

	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ThisClass::Input_CommandSlot1);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ThisClass::Input_CommandSlot2);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ThisClass::Input_CommandSlot3);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ThisClass::Input_CommandSlot4);
	InputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ThisClass::Input_CommandSlot5);
	InputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ThisClass::Input_CommandSlot6);
	InputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &ThisClass::Input_CommandSlot7);
	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &ThisClass::Input_CommandSlot8);
	InputComponent->BindKey(EKeys::Nine, IE_Pressed, this, &ThisClass::Input_CommandSlot9);
	InputComponent->BindKey(EKeys::Zero, IE_Pressed, this, &ThisClass::Input_CommandSlot0);
}

void ATWPlayerController::CreateAndShowHUD()
{
	if (!IsLocalController() || HUDRootWidget)
	{
		return;
	}

	if (!HUDRootWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TWPlayerController] HUDRootWidgetClass is not set."));
		return;
	}

	HUDRootWidget = CreateWidget<UTWHUDRootWidget>(this, HUDRootWidgetClass);
	if (!HUDRootWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TWPlayerController] Failed to create HUDRootWidget."));
		return;
	}

	HUDRootWidget->AddToViewport();
}

void ATWPlayerController::CreateCoordinator()
{
	if (!HUDCoordinator)
	{
		HUDCoordinator = NewObject<UTWUIHUDCoordinator>(this);
	}
}

void ATWPlayerController::BindCoordinator()
{
	if (!HUDCoordinator || !HUDRootWidget || !SelectionProvider || !ResourceProvider)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TWPlayerController] Failed to bind coordinator due to missing references."));
		return;
	}

	HUDCoordinator->Initialize(
		HUDRootWidget,
		SelectionProvider,
		ResourceProvider,
		CommandMetaTable,
		SelectionPresentationTable
	);

	HUDRootWidget->GetOnHUDCommandClickedDelegate().RemoveAll(this);
	HUDRootWidget->GetOnHUDCommandClickedDelegate().AddUObject(this, &ThisClass::HandleCommandButtonClicked);
}

void ATWPlayerController::CreateProviders()
{
	ShutdownProviders();

	SelectionProvider = NewObject<UTWUISelectionProvider>(this);
	ResourceProvider = NewObject<UTWUIResourceProvider>(this);

	if (SelectionProvider)
	{
		SelectionProvider->Initialize(nullptr);
		SelectionProvider->SetUseDebugFallback(true);
	}

	if (ResourceProvider)
	{
		ResourceProvider->Initialize(nullptr);
	}
}

void ATWPlayerController::ShutdownProviders()
{
	if (SelectionProvider)
	{
		SelectionProvider->Shutdown();
		SelectionProvider = nullptr;
	}

	if (ResourceProvider)
	{
		ResourceProvider->Shutdown();
		ResourceProvider = nullptr;
	}
}

void ATWPlayerController::HandleLeftClick()
{
	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Selection] No hit under cursor"));
		return;
	}

	AActor* HitActor = HitResult.GetActor();
	UE_LOG(LogTemp, Warning, TEXT("[Selection] Hit Actor: %s / Class: %s"),
		HitActor ? *HitActor->GetName() : TEXT("None"),
		HitActor ? *HitActor->GetClass()->GetName() : TEXT("None"));

	if (HitActor)
	{
		FSelectionViewModel SelectionVM;
		TArray<FName> CommandIds;
		if (TryBuildSelectionFromActor(HitActor, SelectionVM, CommandIds))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Selection] TryBuildSelectionFromActor Success"));
			SetSelectedActor(HitActor);
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[Selection] TryBuildSelectionFromActor Failed"));
	}

	if (bPendingMoveCommand)
	{
		if (TryIssuePendingMove(HitResult.Location))
		{
			return;
		}
	}

	ClearSelection();
}

void ATWPlayerController::HandleRightClick()
{
	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
	{
		return;
	}

	TryIssuePendingMove(HitResult.Location);
}

void ATWPlayerController::Input_CommandSlot1() { HandleCommandSlotPressed(1); }
void ATWPlayerController::Input_CommandSlot2() { HandleCommandSlotPressed(2); }
void ATWPlayerController::Input_CommandSlot3() { HandleCommandSlotPressed(3); }
void ATWPlayerController::Input_CommandSlot4() { HandleCommandSlotPressed(4); }
void ATWPlayerController::Input_CommandSlot5() { HandleCommandSlotPressed(5); }
void ATWPlayerController::Input_CommandSlot6() { HandleCommandSlotPressed(6); }
void ATWPlayerController::Input_CommandSlot7() { HandleCommandSlotPressed(7); }
void ATWPlayerController::Input_CommandSlot8() { HandleCommandSlotPressed(8); }
void ATWPlayerController::Input_CommandSlot9() { HandleCommandSlotPressed(9); }
void ATWPlayerController::Input_CommandSlot0() { HandleCommandSlotPressed(0); }

void ATWPlayerController::HandleCommandSlotPressed(int32 InHumanReadableHotkey)
{
	TryHandleCommandBySlot(InHumanReadableHotkey);
}

bool ATWPlayerController::TryHandleCommandBySlot(int32 InHumanReadableHotkey)
{
	if (!HUDCoordinator)
	{
		return false;
	}

	const TArray<FCommandSlotViewModel>& CurrentCommands = HUDCoordinator->GetLastBuiltCommandViewModels();
	if (CurrentCommands.Num() == 0)
	{
		return false;
	}

	const int32 WantedSlotIndex = (InHumanReadableHotkey == 0)
		? 9
		: (InHumanReadableHotkey - 1);

	for (const FCommandSlotViewModel& SlotVM : CurrentCommands)
	{
		if (!SlotVM.bVisible || !SlotVM.bEnabled || SlotVM.CommandId.IsNone())
		{
			continue;
		}

		if (SlotVM.SlotIndex == WantedSlotIndex)
		{
			return TryHandleCommandById(SlotVM.CommandId);
		}
	}

	int32 VisibleOrder = 0;
	for (const FCommandSlotViewModel& SlotVM : CurrentCommands)
	{
		if (!SlotVM.bVisible || !SlotVM.bEnabled || SlotVM.CommandId.IsNone())
		{
			continue;
		}

		if (VisibleOrder == WantedSlotIndex)
		{
			return TryHandleCommandById(SlotVM.CommandId);
		}

		++VisibleOrder;
	}

	return false;
}

bool ATWPlayerController::TryHandleCommandById(FName InCommandId)
{
	return ExecuteGameplayCommandById(InCommandId);
}

void ATWPlayerController::HandleCommandButtonClicked(FName InCommandId)
{
	TryHandleCommandById(InCommandId);
}

void ATWPlayerController::SetSelectedActor(AActor* InActor)
{
	UE_LOG(LogTemp, Warning, TEXT("[Selection] SetSelectedActor called: %s"),
		InActor ? *InActor->GetName() : TEXT("None"));

	CurrentSelectedActor = InActor;
	bPendingMoveCommand = false;

	if (!SelectionProvider)
	{
		return;
	}

	FSelectionViewModel SelectionVM;
	TArray<FName> CommandIds;
	if (!TryBuildSelectionFromActor(InActor, SelectionVM, CommandIds))
	{
		ClearSelection();
		return;
	}

	TArray<FSelectionSummaryItemViewModel> SummaryItems;
	SelectionProvider->SetRuntimeSelection(
		SelectionVM,
		CommandIds,
		ESelectionViewMode::Single,
		SelectionVM.SelectionId,
		1,
		SummaryItems
	);

	if (ATWTestBuildingActor* Building = GetSelectedTestBuilding())
	{
		if (HUDCoordinator)
		{
			HUDCoordinator->RefreshCommandPanel();

			TArray<FCommandSlotViewModel> BuiltCommands = HUDCoordinator->GetLastBuiltCommandViewModels();

			for (FCommandSlotViewModel& SlotVM : BuiltCommands)
			{
				SlotVM.QueueCount = Building->GetQueueCountForCommand(SlotVM.CommandId);
			}

			HUDRootWidget->SetCommandData(BuiltCommands);
		}
	}

	PushInfoNotification(FString::Printf(TEXT("Selected: %s"), *SelectionVM.DisplayName));
}

void ATWPlayerController::ClearSelection()
{
	CurrentSelectedActor = nullptr;
	bPendingMoveCommand = false;

	if (SelectionProvider)
	{
		SelectionProvider->ClearRuntimeSelection();
	}
}

bool ATWPlayerController::TryBuildSelectionFromActor(
	AActor* InActor,
	FSelectionViewModel& OutSelectionVM,
	TArray<FName>& OutCommandIds) const
{
	if (const ATWTestUnitActor* Unit = Cast<ATWTestUnitActor>(InActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Selection] Cast Success: TestUnitActor"));

		OutSelectionVM.SelectionType = ESelectionViewType::Unit;
		OutSelectionVM.ViewMode = ESelectionViewMode::Single;
		OutSelectionVM.SelectionId = Unit->GetUIEntityId();
		OutSelectionVM.DisplayName = Unit->GetUIDisplayName();
		OutSelectionVM.TypeLabel = Unit->GetUITypeLabel();
		OutSelectionVM.CurrentHP = Unit->GetCurrentHP();
		OutSelectionVM.MaxHP = Unit->GetMaxHP();
		OutSelectionVM.HPText = FString::Printf(TEXT("%.0f / %.0f"), Unit->GetCurrentHP(), Unit->GetMaxHP());
		OutSelectionVM.TotalSelectedCount = 1;
		OutSelectionVM.CountLabel = TEXT("");

		OutCommandIds = Unit->GetAvailableCommandIds();
		return true;
	}

	if (const ATWTestBuildingActor* Building = Cast<ATWTestBuildingActor>(InActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Selection] Cast Success: TestBuildingActor"));

		OutSelectionVM.SelectionType = ESelectionViewType::Building;
		OutSelectionVM.ViewMode = ESelectionViewMode::Single;
		OutSelectionVM.SelectionId = Building->GetUIEntityId();
		OutSelectionVM.DisplayName = Building->GetUIDisplayName();
		OutSelectionVM.TypeLabel = Building->GetUITypeLabel();
		OutSelectionVM.CurrentHP = Building->GetCurrentHP();
		OutSelectionVM.MaxHP = Building->GetMaxHP();
		OutSelectionVM.HPText = FString::Printf(TEXT("%.0f / %.0f"), Building->GetCurrentHP(), Building->GetMaxHP());
		OutSelectionVM.TotalSelectedCount = 1;
		OutSelectionVM.CountLabel = TEXT("");

		OutCommandIds = Building->GetAvailableCommandIds();
		return true;
	}

	return false;
}

bool ATWPlayerController::TryIssuePendingMove(const FVector& InTargetLocation)
{
	ATWTestUnitActor* Unit = GetSelectedTestUnit();
	if (!Unit)
	{
		bPendingMoveCommand = false;
		return false;
	}

	if (!bPendingMoveCommand)
	{
		Unit->MoveToWorldLocation(InTargetLocation);
		PushInfoNotification(TEXT("Unit moving."));
		return true;
	}

	Unit->MoveToWorldLocation(InTargetLocation);
	bPendingMoveCommand = false;
	PushInfoNotification(TEXT("Move command issued."));
	return true;
}

bool ATWPlayerController::ExecuteGameplayCommandById(FName InCommandId)
{
	if (InCommandId.IsNone())
	{
		return false;
	}

	if (InCommandId == TEXT("BuildMenu") || InCommandId == TEXT("Back"))
	{
		if (SelectionProvider && HUDCoordinator)
		{
			const FUICommandMetaRow* MetaRow = HUDCoordinator->FindCommandMetaRow(InCommandId);
			SelectionProvider->HandleCommandInput(InCommandId, MetaRow);
			HUDCoordinator->RefreshCommandPanel();
			return true;
		}
		return false;
	}

	if (InCommandId == TEXT("BuildBarracks"))
	{
		PushInfoNotification(TEXT("Build Barracks selected."));
		return true;
	}

	if (InCommandId == TEXT("BuildArcheryRange"))
	{
		PushInfoNotification(TEXT("Build Archery Range selected."));
		return true;
	}

	if (InCommandId == TEXT("Move"))
	{
		if (!GetSelectedTestUnit())
		{
			return false;
		}

		bPendingMoveCommand = true;
		PushInfoNotification(TEXT("Move target pending. Click ground."));
		return true;
	}

	if (InCommandId == TEXT("Attack"))
	{
		if (GetSelectedTestUnit())
		{
			PushInfoNotification(TEXT("Attack command selected."));
			return true;
		}

		return false;
	}

	if (InCommandId == TEXT("Hold"))
	{
		if (ATWTestUnitActor* Unit = GetSelectedTestUnit())
		{
			Unit->HoldPosition();
			PushInfoNotification(TEXT("Unit holding position."));
			return true;
		}

		return false;
	}

	if (InCommandId == TEXT("TrainFootman"))
	{
		if (ATWTestBuildingActor* Building = GetSelectedTestBuilding())
		{
			if (Building->StartTrainFootman())
			{
				PushInfoNotification(TEXT("Training Footman..."));

				if (HUDCoordinator && HUDRootWidget)
				{
					HUDCoordinator->RefreshCommandPanel();

					TArray<FCommandSlotViewModel> BuiltCommands = HUDCoordinator->GetLastBuiltCommandViewModels();

					for (FCommandSlotViewModel& SlotVM : BuiltCommands)
					{
						SlotVM.QueueCount = Building->GetQueueCountForCommand(SlotVM.CommandId);
					}

					HUDRootWidget->SetCommandData(BuiltCommands);
				}

				return true;
			}
		}

		return false;
	}

	if (InCommandId == TEXT("TrainArcher"))
	{
		if (ATWTestBuildingActor* Building = GetSelectedTestBuilding())
		{
			if (Building->StartTrainArcher())
			{
				PushInfoNotification(TEXT("Training Archer..."));

				if (HUDCoordinator && HUDRootWidget)
				{
					HUDCoordinator->RefreshCommandPanel();

					TArray<FCommandSlotViewModel> BuiltCommands = HUDCoordinator->GetLastBuiltCommandViewModels();

					for (FCommandSlotViewModel& SlotVM : BuiltCommands)
					{
						SlotVM.QueueCount = Building->GetQueueCountForCommand(SlotVM.CommandId);
					}

					HUDRootWidget->SetCommandData(BuiltCommands);
				}

				return true;
			}
		}

		return false;
	}

	PushInfoNotification(FString::Printf(TEXT("Unhandled Command: %s"), *InCommandId.ToString()));
	return false;
}

ATWTestUnitActor* ATWPlayerController::GetSelectedTestUnit() const
{
	return Cast<ATWTestUnitActor>(CurrentSelectedActor);
}

ATWTestBuildingActor* ATWPlayerController::GetSelectedTestBuilding() const
{
	return Cast<ATWTestBuildingActor>(CurrentSelectedActor);
}

void ATWPlayerController::PushInfoNotification(const FString& InMessage) const
{
	if (HUDCoordinator)
	{
		HUDCoordinator->PushNotification(InMessage, ENotificationType::Info);
	}
}