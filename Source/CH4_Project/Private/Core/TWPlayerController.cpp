#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Building/TWTroopSpawnBuilding.h"
#include "Building/TWPopulationBuilding.h"
#include "Building/TWBlockingBuilding.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "InputMappingContext.h"
#include "EnhancedPlayerInput.h"
#include "MassCommandBuffer.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeFragments.h"
#include "NavigationSystem.h"
#include "Building/GhostBuilding.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/TWGridSubSystem.h"
#include "Subsystems/TWUnitSubsystem.h"

ATWPlayerController::ATWPlayerController()
	: CurrentCommandType(ETWCommand::None)
{
	PrimaryActorTick.bCanEverTick = true;
	SelectedEntities.Empty();
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
	//TODO 다중선택 처리 필요
	FHitResult HitResult;
	if (false == GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		return;
	}
	FVector ClickLocation = HitResult.Location;
	//단일 선택
	if (FVector::DistSquared(ClickLocation, ClickStartLocation) < 50.0f*50.0f)
	{
		ServerHandleSingleSelect(ClickLocation);
	}
	else//다중 선택
	{
		ServerHandleMultipleSelect(ClickStartLocation, ClickLocation);
	}
	
	
}

void ATWPlayerController::OnRightMouseAction(const FInputActionValue& InputActionValue)
{
	//Cancle Command
	if (CurrentCommandType != ETWCommand::None)
	{
		ChangeCurrentCommandType(ETWCommand::None);
		return;
	}
	//Move Command Execute
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
	if (SelectedEntities.IsEmpty())
	{
		return;
	}
	UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
	if (false == IsValid(UnitSubsystem))
	{
		return;
	}

	FMassEntityHandle EntityHandle;
	//TODO 건물이 대상인 경우 처리해야함
	if (UnitSubsystem->FindNearestEntity(CommandLocation, EntityHandle)) //타겟이 유닛인 이동명령
	{
		//TODO 유닛이 대상인 경우 처리해야함
	}
	else //타겟이 지형인 이동명령
	{
		UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
		if (!EntitySubsystem) return;
		UE_LOG(LogMass, Warning, TEXT("MulticastMoveCommand_Implementation"));
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

		FMassCommandBuffer CommandBuffer;
		TWeakObjectPtr<ThisClass> ThisWeakPtr(this);
		CommandBuffer.PushCommand<FMassDeferredSetCommand>(
			[ThisWeakPtr, NavCommandLocation](FMassEntityManager& InOutEntityManager)
			{
				if (false == ThisWeakPtr.IsValid())
				{
					return;
				}
				FMassEntityQuery EntityQuery;
				EntityQuery.Initialize(InOutEntityManager.AsShared());
				EntityQuery.AddRequirement<FTWCommandTypeFragment>(EMassFragmentAccess::ReadWrite);
				EntityQuery.AddRequirement<FMassStateTreeInstanceFragment>(EMassFragmentAccess::ReadOnly);
				EntityQuery.AddSharedRequirement<FTWCommandDataFragment>(EMassFragmentAccess::ReadWrite);

				FMassExecutionContext Context = InOutEntityManager.CreateExecutionContext(0.f);

				for (const FMassEntityHandle& Entity : ThisWeakPtr->SelectedEntities)
				{
					if (!InOutEntityManager.IsEntityActive(Entity)) continue;

					if (FTWCommandTypeFragment* TypeFragment = InOutEntityManager.GetFragmentDataPtr<
						FTWCommandTypeFragment>(Entity))
					{
						TypeFragment->SetType(ETWState::MoveToLocation);
					}
				}

				if (ThisWeakPtr->SelectedEntities.IsValidIndex(0) && InOutEntityManager.IsEntityActive(
					ThisWeakPtr->SelectedEntities[0]))
				{
					if (FTWCommandDataFragment* SharedData = InOutEntityManager.GetSharedFragmentDataPtr<
						FTWCommandDataFragment>(ThisWeakPtr->SelectedEntities[0]))
					{
						SharedData->SetLocation(NavCommandLocation);
					}
				}

				if (UMassSignalSubsystem* SignalSubsystem = InOutEntityManager.GetWorld()->GetSubsystem<
					UMassSignalSubsystem>())
				{
					SignalSubsystem->SignalEntities(UE::Mass::Signals::StateTreeActivate,
					                                ThisWeakPtr->SelectedEntities);
				}
			});

		EntityManager.AppendCommands(TSharedPtr<FMassCommandBuffer>(&CommandBuffer, [](FMassCommandBuffer*)
		{
		}));
	}
}

void ATWPlayerController::ServerHandleAttackCommand_Implementation(const FVector& CommandLocation)
{
	// if (건물)
	// {
	// 	
	// }else if (유닛)
	// {
	// 	
	// }else
	// {
	// 	지형
	// }
}

void ATWPlayerController::ServerHandleHoldCommand_Implementation()
{
	//TODO Hold
}


void ATWPlayerController::ServerHandleSingleSelect_Implementation(const FVector& CommandLocation)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));
	//TODO 건물이 대상인 경우를 먼저 처리해야함
	// if (건물찾기)
	// {
	//	SelectedEntities.Empty();
	// 	return;
	// }
	//
	
	UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
	if (false == IsValid(UnitSubsystem))
	{
		return;
	}

	FMassEntityHandle EntityHandle;
	if (UnitSubsystem->FindNearestEntity(CommandLocation, EntityHandle))
	{
		//SelectedBuilding.Empty();
		SelectedEntities.Empty();
		SelectedEntities.Add(EntityHandle);
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
	TArray<FMassEntityHandle> EntityHandles;
	if (UnitSubsystem->GetEntitiesInRectangle(StartLocation, EndLocation, EntityHandles))
	{
		SelectedEntities.Empty();
		SelectedEntities=EntityHandles;
		//SelectedBuilding.Empty();
		return;
	}
	// TODO
	// if (건물)
	// {
	// 	
	// }
	//	SelectedEntities.Empty();
	
	
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

		TroopBuilding->RequestEnqueueTroop();
		return;
	}
}

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

	for (TActorIterator<ATWBlockingBuilding> It(GetWorld()); It; ++It)
	{
		ATWBlockingBuilding* BlockingBuilding = *It;
		if (!BlockingBuilding)
		{
			continue;
		}

		if (BlockingBuilding->OwnerPlayerSlot == MyPlayerSlot)
		{
			continue;
		}

		BlockingBuilding->ApplyDamageToBuilding(10);
		return;
	}
}


void ATWPlayerController::ChangeCurrentCommandType(ETWCommand CommandType)
{
	//TODO 마우스 커서 변경 등 처리
	CurrentCommandType = CommandType;
}



#pragma endregion

#pragma region 건설

void ATWPlayerController::Server_SpawnBuilding_Implementation(FIntPoint Anchor, FIntPoint BuildSize, TSubclassOf<AActor> ClassToSpawn)
{
	auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();
	
	if (GridSub && GridSub->CanBuildArea(Anchor, BuildSize))
	{
		FVector SpawnPos = GridSub->GetBuildingCenterPosition(Anchor, BuildSize);
		AActor* NewBuilding = GetWorld()->SpawnActor<AActor>(ClassToSpawn, SpawnPos, FRotator::ZeroRotator);
	
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
		if (!BuildClass)
		{
			UE_LOG(LogTemp, Error, TEXT("BuildingClass가 설정안됨"));
			return;
		}
		
		bIsBuildMode = true;
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		CurrentGhost = GetWorld()->SpawnActor<AGhostBuilding>(BuildClass,FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		
		if (CurrentGhost)
		{
			CurrentGhost->BuildingSize = FIntPoint(3,3);
		}
	}
}

void ATWPlayerController::RequestBuild()
{
	if (bIsBuildMode && CurrentGhost)
	{
		Server_SpawnBuilding(CurrentAnchor,CurrentGhost->BuildingSize, SelectedBuildingClass);
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

#pragma endregion
