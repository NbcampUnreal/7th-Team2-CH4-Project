#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Building/TWTroopSpawnBuilding.h"
#include "Building/TWPopulationBuilding.h"
#include "Building/TWNexusBuilding.h"
#include "Building/TWBaseBuilding.h"
#include "Building/TWUpgradeBuilding.h"
#include "Building/TWBlockingBuilding.h"
#include "Building/TWResourceBuilding.h"
#include "Core/TWGameMode.h"

#include "UI/Core/TWPlayerUIBridge.h"
#include "UI/Widgets/TWHUDRootWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "InputMappingContext.h"
#include "EnhancedPlayerInput.h"
#include "MassCommandBuffer.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeFragments.h"
#include "MassNavigationFragments.h"
#include "MassReplicationFragments.h"
#include "MassReplicationSubsystem.h"
#include "NavigationSystem.h"
#include "Building/GhostBuilding.h"
#include "Component/TWBuildComponent.h"
#include "Data/TWBuildingDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/TWGridSubSystem.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "UObject/UnrealType.h"

#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWCommandFragment.h"

ATWPlayerController::ATWPlayerController()
	: CurrentCommandType(ETWCommand::None)
{
	PrimaryActorTick.bCanEverTick = true;
	ServerSelectedEntities.Empty();
	ClientSelectedEntities.Empty();
	bIsBuildMode = false;
	SelectedEntities.Empty();

	BuildComponent = CreateDefaultSubobject<UTWBuildComponent>(TEXT("BuildComponent"));
}

void ATWPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	check(IsValid(DefaultMappingContext));

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	SetShowMouseCursor(false);

	InitializeUIBridge();
	RefreshUIBridge();
}

void ATWPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCursorOverlayPosition();

	const bool bIsEdgeScrollingNow = HandleScreenEdgeScrolling(DeltaSeconds);

	if (bIsEdgeScrollingNow != bWasEdgeScrollingLastFrame)
	{
		if (PlayerUIBridge)
		{
			PlayerUIBridge->SetEdgeScrollingActive(bIsEdgeScrollingNow);
		}

		bWasEdgeScrollingLastFrame = bIsEdgeScrollingNow;
	}

	if (bIsLeftMousePressed && CurrentCommandType == ETWCommand::None && !bIsBuildMode)
	{
		UpdateDragSelectionOverlay();
	}

	if (bIsBuildMode && CurrentGhost)
	{
		FHitResult Hit;

		GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1), true, Hit);

		if (Hit.bBlockingHit)
		{
			auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();
			if (!GridSub)
			{
				return;
			}

			CurrentAnchor = GridSub->WorldToGridPosition(Hit.Location);

			bool bCanBuild = GridSub->CanBuildArea(CurrentAnchor, CurrentGhost->BuildingSize);
			CurrentGhost->UpdateBuildingVisual(bCanBuild);

			FVector CenterPos = GridSub->GetBuildingCenterPosition(CurrentAnchor, CurrentGhost->BuildingSize);
			CurrentGhost->SetActorLocation(CenterPos);
		}
	}
}


#pragma region Input

void ATWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(IsValid(LeftMouseAction));
	check(IsValid(MoveCommandAction));
	check(IsValid(AttackCommandAction));
	check(IsValid(HoldCommandAction));

	check(IsValid(SelectResourceCommandAction));
	check(IsValid(SelectPopulationCommandAction));
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Started, this,
	                                   &ThisClass::OnStartLeftMouseAction);
	EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Completed, this,
	                                   &ThisClass::OnEndLeftMouseAction);
	EnhancedInputComponent->BindAction(RightMouseAction, ETriggerEvent::Started, this, &ThisClass::OnRightMouseAction);
	EnhancedInputComponent->BindAction(MoveCommandAction, ETriggerEvent::Started, this, &ThisClass::OnMoveCommandAction);
	EnhancedInputComponent->BindAction(AttackCommandAction, ETriggerEvent::Started, this, &ThisClass::OnAttackCommandAction);
	EnhancedInputComponent->BindAction(HoldCommandAction, ETriggerEvent::Started, this, &ThisClass::OnHoldCommandAction);
	EnhancedInputComponent->BindAction(SelectResourceCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectWoodBuildingCommandAction);
	EnhancedInputComponent->BindAction(SelectPopulationCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectPopulationBuildingCommandAction);
	if (IA_TestSpawnTroop)
	{
		EnhancedInputComponent->BindAction(
			IA_TestSpawnTroop,
			ETriggerEvent::Started,
			this,
			&ATWPlayerController::HandleTestSpawnTroop
		);
	}

	if (IA_TestIncreasePopulation)
	{
		EnhancedInputComponent->BindAction(
			IA_TestIncreasePopulation,
			ETriggerEvent::Started,
			this,
			&ATWPlayerController::HandleTestIncreasePopulation
		);
	}

	if (IA_TestDamageBlockingBuilding)
	{
		EnhancedInputComponent->BindAction(
			IA_TestDamageBlockingBuilding,
			ETriggerEvent::Started,
			this,
			&ATWPlayerController::HandleTestDamageBlockingBuilding
		);
	}
	
	if (IA_TestUpgrade)
	{
		EnhancedInputComponent->BindAction(
			IA_TestUpgrade,
			ETriggerEvent::Started,
			this,
			&ATWPlayerController::HandleTestUpgrade
		);
	}
}


void ATWPlayerController::OnStartLeftMouseAction(const FInputActionValue& InputActionValue)
{
	if (!BuildComponent->GetBuildMode())
	{
		FHitResult HitResult;
		FVector ClickLocation;
		if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			ClickLocation = HitResult.Location;
		}
		else
		{
			return;
		}
		FHitResult HitResult;
		FVector ClickLocation;
		bIsLeftMousePressed = true;

		// 드래그 UI용 시작 좌표는 DPI 보정 좌표 사용
		FVector2D ScaledMousePos = FVector2D::ZeroVector;
		if (UWidgetLayoutLibrary::GetMousePositionScaledByDPI(this, ScaledMousePos.X, ScaledMousePos.Y))
		{
			DragStartScreenPosition = ScaledMousePos;
		}
		else
		{
			DragStartScreenPosition = LastValidMouseScreenPosition;
		}
	
		if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			ClickLocation = HitResult.Location;
		}
		else
		{
			return;
		}

		if (CurrentCommandType != ETWCommand::None && CurrentCommandType != ETWCommand::Hold)
		{
			switch (CurrentCommandType)
			{
			case ETWCommand::Move:
				ServerHandleMoveCommand(ClickLocation);
				break;
			case ETWCommand::Attack:
				ServerHandleAttackCommand(ClickLocation);
				break;
			default:
				check(false);
				break;
			}
			ChangeCurrentCommandType(ETWCommand::None);
			return;
		}
		ClickStartLocation = ClickLocation;
	}
	else
	{
		if (BuildComponent)
		{
			BuildComponent->RequestBuild();
		}
	}
}

void ATWPlayerController::OnEndLeftMouseAction(const FInputActionValue& InputActionValue)
{
	ChangeCurrentCommandType(ETWCommand::None);
	
	bIsLeftMousePressed = false;
	bIsDraggingSelectionVisual = false;

	if (PlayerUIBridge)
	{
		PlayerUIBridge->SetDragSelectionState(false, FVector2D::ZeroVector, FVector2D::ZeroVector);
	}

	FHitResult HitResult;
	if (false == GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		return;
	}

	FVector ClickLocation = HitResult.Location;

	// 단일 선택
	if (FVector::DistSquared(ClickLocation, ClickStartLocation) < 50.0f * 50.0f)
	{
		if (ATWBaseBuilding* ClickedBuilding = Cast<ATWBaseBuilding>(HitResult.GetActor()))
		{
			ServerHandleBuildingSelect(ClickedBuilding);
			return;
		}

		ServerHandleSingleSelect(ClickLocation);
	}
	else // 다중 선택
	{
		ServerHandleMultipleSelect(ClickStartLocation, ClickLocation);
	}
}

void ATWPlayerController::OnRightMouseAction(const FInputActionValue& InputActionValue)
{
	if (!BuildComponent->GetBuildMode())
	{
		// Cancle Command
		if (CurrentCommandType != ETWCommand::None)
		{
			ChangeCurrentCommandType(ETWCommand::None);
			return;
		}

		// Move Command Execute
		FHitResult HitResult;
		if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			FVector ClickLocation = HitResult.Location;
			ServerHandleMoveCommand(ClickLocation);
		}
	}
	else
	{
		if (BuildComponent)
		{
			BuildComponent->EndBuildMode();
		}
	}
}

inline void ATWPlayerController::OnMoveCommandAction(const FInputActionValue& InputActionValue)
{
	ChangeCurrentCommandType(ETWCommand::Move);
}

void ATWPlayerController::OnAttackCommandAction(const FInputActionValue& InputActionValue)
{
	ChangeCurrentCommandType(ETWCommand::Attack);
}

void ATWPlayerController::OnHoldCommandAction(const FInputActionValue& InputActionValue)
{
	ServerHandleHoldCommand();
}

void ATWPlayerController::OnBuildCommandAction(const FInputActionValue& InputActionValue)
{
	
}

void ATWPlayerController::ServerHandleMoveCommand_Implementation(const FVector& CommandLocation)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));
	if (ServerSelectedEntities.IsEmpty())
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	FNavLocation ProjectedLocation;
	FVector NavCommandLocation = FVector::ZeroVector;
	if (NavSys && NavSys->ProjectPointToNavigation(CommandLocation, ProjectedLocation,
	                                               FVector(500.f, 500.f, 500.f)))
	{
		NavCommandLocation = ProjectedLocation.Location;
	}
	else
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TSharedRef<FMassCommandBuffer> CommandBuffer = MakeShared<FMassCommandBuffer>();
	TWeakObjectPtr<ThisClass> ThisWeakPtr(this);

	CommandBuffer->PushCommand<FMassDeferredSetCommand>(
		[ThisWeakPtr, NavCommandLocation](FMassEntityManager& InOutEntityManager)
		{
			if (false == ThisWeakPtr.IsValid())
			{
				return;
			}

			TArray<FMassEntityHandle> ValidEntities;
			ValidEntities.Reserve(ThisWeakPtr->ServerSelectedEntities.Num());

			for (const FMassEntityHandle& Entity : ThisWeakPtr->ServerSelectedEntities)
			{
				if (!InOutEntityManager.IsEntityActive(Entity))
				{
					continue;
				}

				ValidEntities.Add(Entity);

				if (FTWCommandTypeFragment* TypeFragment = InOutEntityManager.GetFragmentDataPtr<FTWCommandTypeFragment>(Entity))
				{
					TypeFragment->SetType(ETWState::MoveToLocation);
				}

				if (FTWCommandDataFragment* CommandData = InOutEntityManager.GetSharedFragmentDataPtr<FTWCommandDataFragment>(Entity))
				{
					CommandData->SetLocation(NavCommandLocation);
					CommandData->SetTarget(FMassEntityHandle());
				}

				if (FMassMoveTargetFragment* MoveTarget = InOutEntityManager.GetFragmentDataPtr<FMassMoveTargetFragment>(Entity))
				{
					MoveTarget->Center = NavCommandLocation;
				}
			}

			if (UMassSignalSubsystem* SignalSubsystem = InOutEntityManager.GetWorld()->GetSubsystem<UMassSignalSubsystem>())
			{
				if (ValidEntities.Num() > 0)
				{
					SignalSubsystem->SignalEntities(UE::Mass::Signals::StateTreeActivate, ValidEntities);
				}
			}
		});

	EntityManager.AppendCommands(CommandBuffer);

	UE_LOG(LogTemp, Log, TEXT("[Move] SelectedEntities=%d / Target=%s"),
	       ServerSelectedEntities.Num(),
	       *NavCommandLocation.ToString());
}

void ATWPlayerController::ServerHandleAttackCommand_Implementation(const FVector& CommandLocation)
{
	// TODO
}

void ATWPlayerController::ServerHandleHoldCommand_Implementation()
{
	// TODO Hold
	checkf(HasAuthority(), TEXT("Server Logic Called!"));
	if (ServerSelectedEntities.IsEmpty())
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	TSharedRef<FMassCommandBuffer> CommandBuffer = MakeShared<FMassCommandBuffer>();
	TWeakObjectPtr<ThisClass> ThisWeakPtr(this);

	CommandBuffer->PushCommand<FMassDeferredSetCommand>(
		[ThisWeakPtr](FMassEntityManager& InOutEntityManager)
		{
			if (false == ThisWeakPtr.IsValid())
			{
				return;
			}

			TArray<FMassEntityHandle> ValidEntities;
			ValidEntities.Reserve(ThisWeakPtr->ServerSelectedEntities.Num());

			for (const FMassEntityHandle& Entity : ThisWeakPtr->ServerSelectedEntities)
			{
				if (!InOutEntityManager.IsEntityActive(Entity))
				{
					continue;
				}

				ValidEntities.Add(Entity);

				if (FTWCommandTypeFragment* TypeFragment = InOutEntityManager.GetFragmentDataPtr<FTWCommandTypeFragment>(Entity))
				{
					TypeFragment->SetType(ETWState::Hold);
				}
			}

			if (UMassSignalSubsystem* SignalSubsystem = InOutEntityManager.GetWorld()->GetSubsystem<UMassSignalSubsystem>())
			{
				if (ValidEntities.Num() > 0)
				{
					SignalSubsystem->SignalEntities(UE::Mass::Signals::StateTreeActivate, ValidEntities);
				}
			}
		});

	EntityManager.AppendCommands(CommandBuffer);
}

void ATWPlayerController::ServerHandleSingleSelect_Implementation(const FVector& CommandLocation)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));

	UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
	if (false == IsValid(UnitSubsystem))
	{
		return;
	}

	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	FMassEntityHandle EntityHandle;
	if (UnitSubsystem->FindNearestEntity(CommandLocation, EntityHandle, TWPS->PlayerSlot))
	{
		SelectedBuilding = nullptr;
		ServerSelectedEntities.Empty();
		ServerSelectedEntities.Add(EntityHandle);

		TArray<FMassNetworkID> NetworkIds;
		float PrimaryHealth = 0.f;
		bool bHasPrimaryHealth = false;
		BuildSelectionPayloadFromEntities(ServerSelectedEntities, NetworkIds, PrimaryHealth, bHasPrimaryHealth);

		ClientApplyUnitSelection(NetworkIds, PrimaryHealth, bHasPrimaryHealth);
	}
	else
	{
		ServerSelectedEntities.Empty();
		SelectedBuilding = nullptr;
		ClientClearSelection();
	}
}

void ATWPlayerController::ServerHandleMultipleSelect_Implementation(const FVector& StartLocation,
	const FVector& EndLocation)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));

	UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
	if (false == IsValid(UnitSubsystem))
	{
		return;
	}

	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	TArray<FMassEntityHandle> EntityHandles;
	if (UnitSubsystem->GetEntitiesInRectangle(StartLocation, EndLocation, EntityHandles, TWPS->PlayerSlot))
	{
		ServerSelectedEntities = EntityHandles;
		SelectedBuilding = nullptr;

		TArray<FMassNetworkID> UnitIds;
		float PrimaryHealth = 0.f;
		bool bHasPrimaryHealth = false;
		BuildSelectionPayloadFromEntities(ServerSelectedEntities, UnitIds, PrimaryHealth, bHasPrimaryHealth);

		ClientApplyUnitSelection(UnitIds, PrimaryHealth, bHasPrimaryHealth);
		return;
	}

	ServerSelectedEntities.Empty();
	SelectedBuilding = nullptr;
	ClientClearSelection();
}

void ATWPlayerController::ServerHandleBuildingSelect_Implementation(ATWBaseBuilding* TargetBuilding)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));

	ServerSelectedEntities.Empty();
	SelectedBuilding = TargetBuilding;

	if (!IsValid(TargetBuilding))
	{
		ClientClearSelection();
		return;
	}

	ClientApplyBuildingSelection(TargetBuilding);
}

bool ATWPlayerController::HandleScreenEdgeScrolling(float DeltaSeconds)
{
	if (!IsLocalController())
	{
		return false;
	}

	if (!bHasValidRawMousePositionThisFrame)
	{
		return false;
	}

	const FVector2D& MousePosition = CurrentFrameRawMousePosition;

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	FVector MoveDirection = FVector::ZeroVector;

	// Left / Right
	if (MousePosition.X <= ScreenEdgeEnterMargin)
	{
		MoveDirection.Y = -1.f;
	}
	else if (MousePosition.X >= static_cast<float>(ViewportSizeX) - ScreenEdgeEnterMargin)
	{
		MoveDirection.Y = 1.f;
	}

	// Top / Bottom
	if (MousePosition.Y <= ScreenEdgeEnterMargin)
	{
		MoveDirection.X = 1.f;
	}
	else if (MousePosition.Y >= static_cast<float>(ViewportSizeY) - ScreenEdgeEnterMargin)
	{
		MoveDirection.X = -1.f;
	}

	const bool bIsEdgeScrollingNow = !MoveDirection.IsNearlyZero();

	if (bIsEdgeScrollingNow)
	{
		if (APawn* MyPawn = GetPawn())
		{
			MyPawn->AddMovementInput(MoveDirection.GetSafeNormal(), ScrollSpeed * DeltaSeconds);
		}
	}

	return bIsEdgeScrollingNow;
}

void ATWPlayerController::ChangeCurrentCommandType(ETWCommand CommandType)
{
	CurrentCommandType = CommandType;
	UpdateInputOverlayState();
}

#pragma endregion

#pragma region UI

void ATWPlayerController::InitializeUIBridge()
{
	if (false == IsLocalController())
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
		this,
		HUDRootWidgetClass,
		CommandMetaTable,
		SelectionPresentationTable,
		bUseUIDebugFallback,
		DefaultSelectedUnitId,
		DefaultSelectedBuildingId,
		DefaultMultiSelectedUnitId
	);

	PlayerUIBridge->GetOnUICommandRequestedDelegate().RemoveAll(this);
	PlayerUIBridge->GetOnUICommandRequestedDelegate().AddUObject(this, &ATWPlayerController::HandleUICommandRequested);
}

void ATWPlayerController::RefreshUIBridge()
{
	if (PlayerUIBridge)
	{
		PlayerUIBridge->RefreshAll();
	}
}

void ATWPlayerController::ClearLocalSelectionCache()
{
	SelectedBuilding = nullptr;
	LocalSelectedUnitCount = 0;
	LocalPrimarySelectedUnitId = NAME_None;
	bHasLocalPrimarySelectedUnitStatus = false;
	LocalSelectionSummaryItems.Empty();
}

void ATWPlayerController::RebuildLocalSelectionSummary(const TArray<FName>& InUnitIds)
{
	LocalSelectionSummaryItems.Empty();

	TMap<FName, int32> CountMap;
	for (const FName& UnitId : InUnitIds)
	{
		CountMap.FindOrAdd(UnitId)++;
	}

	for (const TPair<FName, int32>& Pair : CountMap)
	{
		FSelectionSummaryItemViewModel Item;
		Item.EntityId = Pair.Key;
		Item.DisplayName = TEXT("");
		Item.Count = Pair.Value;
		Item.CountText = FString::Printf(TEXT("x%d"), Pair.Value);
		LocalSelectionSummaryItems.Add(Item);
	}
}

void ATWPlayerController::BuildSelectionPayloadFromEntities(
	const TArray<FMassEntityHandle>& InSelectedEntities,
	TArray<FMassNetworkID>& OutNetworkIds,
	float& OutPrimaryHealth,
	bool& bOutHasPrimaryHealth)
{
	OutNetworkIds.Empty();
	OutPrimaryHealth = 0.f;
	bOutHasPrimaryHealth = false;

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	for (int32 Index = 0; Index < InSelectedEntities.Num(); ++Index)
	{
		const FMassEntityHandle& Entity = InSelectedEntities[Index];
		if (!EntityManager.IsEntityActive(Entity))
		{
			continue;
		}
		
		if (FMassNetworkIDFragment* NetworkIDFragmentFragment = EntityManager.GetFragmentDataPtr<FMassNetworkIDFragment>(Entity))
		{
			OutNetworkIds.Add(NetworkIDFragmentFragment->NetID);
		}
		
		
		

		if (Index == 0)
		{
			if (FTWStatusFragment* StatusFragment = EntityManager.GetFragmentDataPtr<FTWStatusFragment>(Entity))
			{
				const float Health = StatusFragment->GetMutableStatus().GetStatus(ETWStatusType::Health);
				OutPrimaryHealth = Health;
				bOutHasPrimaryHealth = true;
			}
		}
	}
}

const FUICommandMetaRow* ATWPlayerController::FindCommandMetaRowFromTable(FName CommandId) const
{
	if (CommandId.IsNone() || !CommandMetaTable)
	{
		return nullptr;
	}

	static const FString ContextString(TEXT("ATWPlayerController::FindCommandMetaRowFromTable"));
	return CommandMetaTable->FindRow<FUICommandMetaRow>(CommandId, ContextString);
}

void ATWPlayerController::ClientApplyUnitSelection_Implementation(
	const TArray<FMassNetworkID>& InNetworkIds,
	float InPrimaryHealth,
	bool bInHasPrimaryHealth)
{
	ClearLocalSelectionCache();

	LocalSelectedUnitCount = InNetworkIds.Num();
	
	ClientSelectedEntities = InNetworkIds;
	UMassReplicationSubsystem* RepSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
	TArray<FName> InUnitIds;

	//우선 기존 코드 방식 유지해뒀어요
	//리슨서버에서는 ServerSelectedUnits에서 정보를 얻어와야해요
	for (const FMassNetworkID& NetID : InNetworkIds)
	{
					

		UE_LOG(LogTemp, Warning, TEXT("%d"), NetID.GetValue());
		
		const FMassReplicationEntityInfo* EntityInfo = RepSubsystem->GetEntityInfoMap().Find(NetID);
		if (!EntityInfo)
		{
			continue;
		}
		FMassEntityHandle EntityHandle = EntityInfo->Entity;
		FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(EntityHandle);
		check(UnitFragment != nullptr);
		
		InUnitIds.Add(UnitFragment->GetUnitID());
	}
	
	if (InUnitIds.Num() > 0)
	{

		LocalPrimarySelectedUnitId = InUnitIds[0];
		RebuildLocalSelectionSummary(InUnitIds);
	}

	if (bInHasPrimaryHealth)
	{
		LocalPrimarySelectedUnitStatus = FTWUnitStatus();
		LocalPrimarySelectedUnitStatus.SetStatus(ETWStatusType::Health, InPrimaryHealth);
		bHasLocalPrimarySelectedUnitStatus = true;
	}

	RefreshUIBridge();
}

void ATWPlayerController::ClientApplyBuildingSelection_Implementation(ATWBaseBuilding* InBuilding)
{
	ClearLocalSelectionCache();
	SelectedBuilding = InBuilding;
	RefreshUIBridge();
}

void ATWPlayerController::ClientClearSelection_Implementation()
{
	ClearLocalSelectionCache();
	RefreshUIBridge();
}

void ATWPlayerController::ClientForceRefreshSelectionBridge_Implementation()
{
	if (PlayerUIBridge)
	{
		PlayerUIBridge->ForceRefreshSelectionFromGameplayEvent();
	}
	else
	{
		RefreshUIBridge();
	}
}
void ATWPlayerController::HandleUICommandRequested(FName CommandId)
{
	UE_LOG(LogTemp, Log, TEXT("[UI Click] HandleUICommandRequested: %s"), *CommandId.ToString());

	if (CommandId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI Click] Ignored: CommandId is None"));
		return;
	}

	if (PlayerUIBridge && PlayerUIBridge->HandleContextCommand(CommandId))
	{
		UE_LOG(LogTemp, Log, TEXT("[UI Click] Consumed by HandleContextCommand: %s"), *CommandId.ToString());
		RefreshUIBridge();
		return;
	}

	const FUICommandMetaRow* CommandMeta = nullptr;

	if (PlayerUIBridge)
	{
		CommandMeta = PlayerUIBridge->FindCommandMetaRow(CommandId);
	}

	if (!CommandMeta)
	{
		CommandMeta = FindCommandMetaRowFromTable(CommandId);
	}

	if (!CommandMeta)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI Click] CommandMeta not found: %s"), *CommandId.ToString());
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[UI Click] Resolved CommandMeta: CommandId=%s / Type=%d / Route=%d / PayloadId=%s"),
		*CommandId.ToString(),
		static_cast<int32>(CommandMeta->CommandType),
		static_cast<int32>(CommandMeta->CommandRoute),
		*CommandMeta->PayloadId.ToString()
	);

	switch (CommandMeta->CommandType)
	{
	case ETWCommandType::Move:
		ChangeCurrentCommandType(ETWCommand::Move);
		UE_LOG(LogTemp, Log, TEXT("[UI Click] Move command armed"));
		return;

	case ETWCommandType::Attack:
		ChangeCurrentCommandType(ETWCommand::Attack);
		UE_LOG(LogTemp, Log, TEXT("[UI Click] Attack command armed"));
		return;

	case ETWCommandType::Hold:
		UE_LOG(LogTemp, Log, TEXT("[UI Click] Hold command -> server"));
		ServerHandleHoldCommand();
		return;

	case ETWCommandType::ProduceUnit:
	case ETWCommandType::BuildStructure:
	case ETWCommandType::Research:
		UE_LOG(LogTemp, Log, TEXT("[UI Click] Gameplay command -> server: %s"), *CommandId.ToString());
		ServerHandleUICommandRequested(CommandId);
		return;

	default:
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI Click] Unsupported command type: CommandId=%s / Type=%d"),
			*CommandId.ToString(),
			static_cast<int32>(CommandMeta->CommandType)
		);
		return;
	}
}

void ATWPlayerController::ServerHandleUICommandRequested_Implementation(FName CommandId)
{
	if (CommandId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI Produce] Server ignored: CommandId is None"));
		return;
	}

	ATWBaseBuilding* CurrentSelectedBuilding = GetSelectedBuilding();
	if (!IsValid(CurrentSelectedBuilding))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI Produce] Server ignored: SelectedBuilding is invalid / CommandId=%s"), *CommandId.ToString());
		return;
	}

	const FUICommandMetaRow* CommandMeta = FindCommandMetaRowFromTable(CommandId);
	if (!CommandMeta)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI Produce] Server ignored: CommandMeta not found / CommandId=%s"), *CommandId.ToString());
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[UI Produce] ServerHandleUICommandRequested: Building=%s / CommandId=%s / Type=%d / PayloadId=%s"),
		*CurrentSelectedBuilding->GetName(),
		*CommandId.ToString(),
		static_cast<int32>(CommandMeta->CommandType),
		*CommandMeta->PayloadId.ToString()
	);

	switch (CommandMeta->CommandType)
	{
	case ETWCommandType::ProduceUnit:
	{
		bool bProduced = false;

		if (TryInvokeBuildingProduceById(CurrentSelectedBuilding, CommandMeta->PayloadId))
		{
			bProduced = true;

			UE_LOG(
				LogTemp,
				Log,
				TEXT("[UI Produce] ProduceById success: Building=%s / PayloadId=%s"),
				*CurrentSelectedBuilding->GetName(),
				*CommandMeta->PayloadId.ToString()
			);
		}
		else if (TryInvokeBuildingProduceFallback(CurrentSelectedBuilding))
		{
			bProduced = true;

			UE_LOG(
				LogTemp,
				Log,
				TEXT("[UI Produce] ProduceFallback success: Building=%s / CommandId=%s"),
				*CurrentSelectedBuilding->GetName(),
				*CommandId.ToString()
			);
		}

		if (bProduced)
		{
			RefreshUIBridge();
			ClientForceRefreshSelectionBridge();
			return;
		}

		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI Produce] Enqueue failed on building: %s / CommandId=%s / PayloadId=%s / Class=%s"),
			*CurrentSelectedBuilding->GetName(),
			*CommandId.ToString(),
			*CommandMeta->PayloadId.ToString(),
			*CurrentSelectedBuilding->GetClass()->GetName()
		);
		return;
	}

	case ETWCommandType::BuildStructure:
	case ETWCommandType::Research:
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI Produce] Command type reached server but implementation is not completed: CommandId=%s / Type=%d"),
			*CommandId.ToString(),
			static_cast<int32>(CommandMeta->CommandType)
		);
		return;

	default:
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI Produce] Unsupported server command type: CommandId=%s / Type=%d"),
			*CommandId.ToString(),
			static_cast<int32>(CommandMeta->CommandType)
		);
		return;
	}
}

bool ATWPlayerController::TryInvokeBuildingProduceById(ATWBaseBuilding* TargetBuilding, FName UnitId) const
{
	if (!IsValid(TargetBuilding) || UnitId.IsNone())
	{
		return false;
	}

	struct FProduceByIdParams
	{
		FName UnitId = NAME_None;
		bool ReturnValue = false;
	};

	static const TArray<FName> CandidateFunctionNames = {
		TEXT("RequestEnqueueTroopById"),
		TEXT("RequestEnqueueUnitById"),
		TEXT("EnqueueTroopById"),
		TEXT("EnqueueUnitById")
	};

	for (const FName& FunctionName : CandidateFunctionNames)
	{
		UFunction* FoundFunction = TargetBuilding->FindFunction(FunctionName);
		if (!FoundFunction)
		{
			continue;
		}

		TArray<FProperty*> InputParams;
		FProperty* ReturnProperty = nullptr;

		for (TFieldIterator<FProperty> It(FoundFunction); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Parm))
			{
				continue;
			}

			if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnProperty = Property;
				continue;
			}

			InputParams.Add(Property);
		}

		if (InputParams.Num() != 1)
		{
			continue;
		}

		if (!InputParams[0]->IsA(FNameProperty::StaticClass()))
		{
			continue;
		}

		if (!ReturnProperty || !ReturnProperty->IsA(FBoolProperty::StaticClass()))
		{
			continue;
		}

		FProduceByIdParams Params;
		Params.UnitId = UnitId;
		Params.ReturnValue = false;

		TargetBuilding->ProcessEvent(FoundFunction, &Params);
		return Params.ReturnValue;
	}

	return false;
}

bool ATWPlayerController::TryInvokeBuildingProduceFallback(ATWBaseBuilding* TargetBuilding) const
{
	if (!IsValid(TargetBuilding))
	{
		return false;
	}

	struct FProduceFallbackParams
	{
		bool ReturnValue = false;
	};

	static const TArray<FName> CandidateFunctionNames = {
		TEXT("RequestEnqueueTroop"),
		TEXT("RequestEnqueueUnit"),
		TEXT("EnqueueTroop"),
		TEXT("EnqueueUnit")
	};

	for (const FName& FunctionName : CandidateFunctionNames)
	{
		UFunction* FoundFunction = TargetBuilding->FindFunction(FunctionName);
		if (!FoundFunction)
		{
			continue;
		}

		int32 InputParamCount = 0;
		FProperty* ReturnProperty = nullptr;

		for (TFieldIterator<FProperty> It(FoundFunction); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Parm))
			{
				continue;
			}

			if (Property->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnProperty = Property;
				continue;
			}

			++InputParamCount;
		}

		if (InputParamCount != 0)
		{
			continue;
		}

		if (!ReturnProperty || !ReturnProperty->IsA(FBoolProperty::StaticClass()))
		{
			continue;
		}

		FProduceFallbackParams Params;
		Params.ReturnValue = false;

		TargetBuilding->ProcessEvent(FoundFunction, &Params);
		return Params.ReturnValue;
	}

	return false;
}

FName ATWPlayerController::ResolveBuildingSelectionId(const ATWBaseBuilding* InBuilding) const
{
	if (!IsValid(InBuilding))
	{
		return DefaultSelectedBuildingId;
	}

	if (Cast<ATWTroopSpawnBuilding>(InBuilding))
	{
		return TEXT("TroopSpawnBuilding");
	}

	if (Cast<ATWPopulationBuilding>(InBuilding))
	{
		return TEXT("PopulationBuilding");
	}

	if (Cast<ATWResourceBuilding>(InBuilding))
	{
		return TEXT("ResourceBuilding");
	}

	if (Cast<ATWBlockingBuilding>(InBuilding))
	{
		return TEXT("BlockingBuilding");
	}

	if (Cast<ATWNexusBuilding>(InBuilding))
	{
		return TEXT("NexusBuilding");
	}

	return InBuilding->GetClass()->GetFName();
}

#pragma endregion

#pragma region 병력 스폰 대기열
void ATWPlayerController::HandleTestSpawnTroop(const FInputActionValue& Value)
{
	ServerTestSpawnTroop();
}

void ATWPlayerController::ServerTestSpawnTroop_Implementation()
{
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	ATWTroopSpawnBuilding* TargetTroopBuilding = nullptr;

	if (ATWBaseBuilding* CurrentSelectedBuilding = GetSelectedBuilding())
	{
		TargetTroopBuilding = Cast<ATWTroopSpawnBuilding>(CurrentSelectedBuilding);
	}

	if (!TargetTroopBuilding)
	{
		const int32 MyPlayerSlot = TWPS->PlayerSlot;

		for (TActorIterator<ATWTroopSpawnBuilding> It(GetWorld()); It; ++It)
		{
			ATWTroopSpawnBuilding* TroopBuilding = *It;
			if (!TroopBuilding)
			{
				continue;
			}

			if (TroopBuilding->OwnerPlayerSlot != MyPlayerSlot)
			{
				continue;
			}

			TargetTroopBuilding = TroopBuilding;
			break;
		}
	}

	if (!TargetTroopBuilding)
	{
		return;
	}

	const bool bEnqueueSuccess = TargetTroopBuilding->RequestEnqueueTroop();
	if (bEnqueueSuccess)
	{
		RefreshUIBridge();
		ClientForceRefreshSelectionBridge();
	}
}
#pragma endregion

#pragma region 인구 수 대기열	
void ATWPlayerController::HandleTestIncreasePopulation(const FInputActionValue& Value)
{
	ServerTestIncreasePopulation();
}

void ATWPlayerController::ServerTestIncreasePopulation_Implementation()
{
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

	for (TActorIterator<ATWPopulationBuilding> It(GetWorld()); It; ++It)
	{
		ATWPopulationBuilding* PopulationBuilding = *It;
		if (!PopulationBuilding)
		{
			continue;
		}

		if (PopulationBuilding->OwnerPlayerSlot != MyPlayerSlot)
		{
			continue;
		}

		PopulationBuilding->RequestEnqueuePopulation();
		return;
	}
}
#pragma endregion

#pragma region 넥서스 데미지
void ATWPlayerController::HandleTestDamageBlockingBuilding(const FInputActionValue& Value)
{
	ServerTestDamageBlockingBuilding();
}

void ATWPlayerController::ServerTestDamageBlockingBuilding_Implementation()
{
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

	for (TActorIterator<ATWNexusBuilding> It(GetWorld()); It; ++It)
	{
		ATWNexusBuilding* NexusBuilding = *It;
		if (!NexusBuilding)
		{
			continue;
		}

		if (NexusBuilding->OwnerPlayerSlot == MyPlayerSlot)
		{
			continue;
		}

		NexusBuilding->ApplyDamageToBuilding(10);
		return;
	}
}
#pragma endregion

#pragma region 건설
void ATWPlayerController::OnRequestBuildCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->RequestBuild();
	}
}

void ATWPlayerController::OnCancelBuildCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->EndBuildMode();
	}
}

void ATWPlayerController::OnSelectWoodBuildingCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->SelectBuildingToConstruct(EBuildingCategory::ResourceProduction);
	}
}

void ATWPlayerController::OnSelectStoneBuildingCommandAction()
{
	
}

void ATWPlayerController::OnSelectPopulationBuildingCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->SelectBuildingToConstruct(EBuildingCategory::PopulationProduction);
	}
}

void ATWPlayerController::OnSelectTroopBuildingCommandAction()
{
}

void ATWPlayerController::OnSelectUpgradeBuildingCommandAction()
{
}

void ATWPlayerController::OnSelectBlockingBuildingCommandAction()
{
}

void ATWPlayerController::NotifyResourceStateChanged()
{
	if (false == IsLocalController())
	{
		return;
	}

	RefreshUIBridge();
}

#pragma endregion

#pragma region 업그레이드
void ATWPlayerController::HandleTestUpgrade(const FInputActionValue& Value)
{
	ServerTestUpgrade();
}

void ATWPlayerController::ServerTestUpgrade_Implementation()
{
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

	for (TActorIterator<ATWUpgradeBuilding> It(GetWorld()); It; ++It)
	{
		ATWUpgradeBuilding* UpgradeBuilding = *It;
		if (!UpgradeBuilding)
		{
			continue;
		}

		if (UpgradeBuilding->OwnerPlayerSlot != MyPlayerSlot)
		{
			continue;
		}

		UpgradeBuilding->RequestStartUpgrade(TEXT("Upgrade_Damage"));
		return;
	}
}
#pragma endregion

#pragma region Input Overlay

void ATWPlayerController::UpdateInputOverlayState()
{
	if (!PlayerUIBridge)
	{
		return;
	}

	const FName ArmedCommandId = ConvertCommandTypeToCommandId(CurrentCommandType);

	if (ArmedCommandId.IsNone())
	{
		PlayerUIBridge->ClearArmedCommandState();
	}
	else
	{
		PlayerUIBridge->SetArmedCommandState(ArmedCommandId);
	}
}

void ATWPlayerController::UpdateDragSelectionOverlay()
{
	if (!PlayerUIBridge)
	{
		return;
	}

	FVector2D ScaledMousePos = FVector2D::ZeroVector;
	bool bHasScaledMouse =
		UWidgetLayoutLibrary::GetMousePositionScaledByDPI(this, ScaledMousePos.X, ScaledMousePos.Y);

	if (bHasScaledMouse)
	{
		CurrentMouseScreenPosition = ScaledMousePos;
	}
	else
	{
		CurrentMouseScreenPosition = LastValidMouseScreenPosition;
	}

	const float Distance = FVector2D::Distance(DragStartScreenPosition, CurrentMouseScreenPosition);
	bIsDraggingSelectionVisual = (Distance >= DragSelectionScreenThreshold);

	PlayerUIBridge->SetDragSelectionState(
		bIsDraggingSelectionVisual,
		DragStartScreenPosition,
		CurrentMouseScreenPosition
	);
}

void ATWPlayerController::UpdateCursorOverlayPosition()
{
	if (!PlayerUIBridge)
	{
		return;
	}

	// 1) 커서 표시용 좌표 (DPI 보정)
	FVector2D ScaledMousePos = FVector2D::ZeroVector;
	bHasValidMousePositionThisFrame =
		UWidgetLayoutLibrary::GetMousePositionScaledByDPI(this, ScaledMousePos.X, ScaledMousePos.Y);

	if (bHasValidMousePositionThisFrame)
	{
		CurrentFrameMouseScreenPosition = ScaledMousePos;
		LastValidMouseScreenPosition = ScaledMousePos;
	}

	// 2) 엣지 스크롤 판정용 좌표 (raw viewport)
	FVector2D RawMousePos = FVector2D::ZeroVector;
	bHasValidRawMousePositionThisFrame = GetMousePosition(RawMousePos.X, RawMousePos.Y);

	if (bHasValidRawMousePositionThisFrame)
	{
		CurrentFrameRawMousePosition = RawMousePos;
	}

	// 커서는 마지막 정상 위치 유지
	PlayerUIBridge->SetCursorScreenPosition(LastValidMouseScreenPosition);
}

FName ATWPlayerController::ConvertCommandTypeToCommandId(ETWCommand InCommandType) const
{
	switch (InCommandType)
	{
	case ETWCommand::Move:
		return TEXT("Move");

	case ETWCommand::Attack:
		return TEXT("Attack");

	default:
		return NAME_None;
	}
}

#pragma endregion

