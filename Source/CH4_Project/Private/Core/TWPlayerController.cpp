#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Building/TWTroopSpawnBuilding.h"
#include "Building/TWPopulationBuilding.h"
#include "Building/TWNexusBuilding.h"
#include "Building/TWBaseBuilding.h"
#include "Building/TWBlockingBuilding.h"
#include "Building/TWResourceBuilding.h"
#include "Core/TWGameMode.h"

#include "UI/Core/TWPlayerUIBridge.h"
#include "UI/Widgets/TWHUDRootWidget.h"
#include "UI/Data/TWUIDataTypes.h"

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
	SetShowMouseCursor(true);

	InitializeUIBridge();
	RefreshUIBridge();
}

void ATWPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	HandleScreenEdgeScrolling(DeltaSeconds);

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
	check(IsValid(BuildCommandAction));

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
	EnhancedInputComponent->BindAction(BuildCommandAction, ETriggerEvent::Started, this, &ThisClass::OnBuildCommandAction);

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
}


void ATWPlayerController::OnStartLeftMouseAction(const FInputActionValue& InputActionValue)
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
	//RequestBuild();
}

void ATWPlayerController::OnEndLeftMouseAction(const FInputActionValue& InputActionValue)
{
	ChangeCurrentCommandType(ETWCommand::None);

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
	ToggleBuildMode();
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

void ATWPlayerController::HandleScreenEdgeScrolling(float DeltaSeconds)
{
	if (false == IsLocalController())
	{
		return;
	}
	FVector2D MousePosition;
	if (false == GetMousePosition(MousePosition.X, MousePosition.Y))
	{
		return;
	}
	int32 ViewportSizeX, ViewportSizeY;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	FVector MoveDirection = FVector::ZeroVector;

	if (MousePosition.X < ScreenEdgeMargin)
	{
		MoveDirection.Y = -1;
	}
	else if (MousePosition.X > ViewportSizeX - ScreenEdgeMargin)
	{
		MoveDirection.Y = 1;
	}
	if (MousePosition.Y < ScreenEdgeMargin)
	{
		MoveDirection.X = 1;
	}
	else if (MousePosition.Y > ViewportSizeY - ScreenEdgeMargin)
	{
		MoveDirection.X = -1;
	}

	if (false == MoveDirection.IsNearlyZero())
	{
		APawn* MyPawn = GetPawn();
		if (MyPawn)
		{
			MyPawn->AddMovementInput(MoveDirection.GetSafeNormal(), ScrollSpeed * DeltaSeconds);
		}
	}
}

void ATWPlayerController::ChangeCurrentCommandType(ETWCommand CommandType)
{
	// TODO 마우스 커서 변경 등 처리
	CurrentCommandType = CommandType;
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

void ATWPlayerController::Server_SpawnBuilding_Implementation(FIntPoint Anchor, FIntPoint BuildSize, TSubclassOf<ATWBaseBuilding> ClassToSpawn)
{
	auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();

	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	if (GridSub && GridSub->CanBuildArea(Anchor, BuildSize))
	{
		FVector SpawnPos = GridSub->GetBuildingCenterPosition(Anchor, BuildSize);
		ATWBaseBuilding* NewBuilding = GetWorld()->SpawnActor<ATWBaseBuilding>(ClassToSpawn, SpawnPos, FRotator::ZeroRotator);
		if (!NewBuilding)
		{
			return;
		}

		NewBuilding->OwnerPlayerSlot = TWPS->PlayerSlot;

		if (ATWGameMode* TWGM = GetWorld()->GetAuthGameMode<ATWGameMode>())
		{
			TWGM->TryBindBuilding(NewBuilding);
		}

		GridSub->OccupyArea(Anchor, BuildSize, NewBuilding);
	}
}

void ATWPlayerController::ToggleBuildMode()
{
	if (bIsBuildMode)
	{
		EndBuildMode();
	}
	else
	{
		if (!BuildClass || !SelectedBuildingClass)
		{
			UE_LOG(LogTemp, Error, TEXT("BuildingClass가 설정안됨"));
			return;
		}

		ATWBaseBuilding* DefaultBuilding = SelectedBuildingClass->GetDefaultObject<ATWBaseBuilding>();

		if (!DefaultBuilding || !DefaultBuilding->BuildingData)
		{
			UE_LOG(LogTemp, Error, TEXT("DA BuildingData이 없음"));
			return;
		}
		bIsBuildMode = true;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentGhost = GetWorld()->SpawnActor<AGhostBuilding>(BuildClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentGhost)
		{
			CurrentGhost->BuildingSize = DefaultBuilding->BuildingData->GridSize.BuildingSize;
		}
	}
}

void ATWPlayerController::RequestBuild()
{
	if (bIsBuildMode && CurrentGhost)
	{
		Server_SpawnBuilding(CurrentAnchor, CurrentGhost->BuildingSize, SelectedBuildingClass);

		auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();
		if (GridSub)
		{
			GridSub->OccupyArea(CurrentAnchor, CurrentGhost->BuildingSize, nullptr);
		}

		EndBuildMode();
	}
}

void ATWPlayerController::EndBuildMode()
{
	bIsBuildMode = false;

	if (CurrentGhost != nullptr)
	{
		CurrentGhost->Destroy();
		CurrentGhost = nullptr;
	}
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