#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"

#include "Building/TWTroopSpawnBuilding.h"
#include "Building/TWPopulationBuilding.h"
#include "Building/TWNexusBuilding.h"
#include "Building/TWBaseBuilding.h"
#include "Building/TWUpgradeBuilding.h"
#include "Building/TWBlockingBuilding.h"
#include "Building/TWResourceBuilding.h"

#include "UI/Core/TWPlayerUIBridge.h"
#include "UI/Data/TWUIDataTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "NavigationSystem.h"
#include "Component/TWBuildComponent.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "UObject/UnrealType.h"

#include "MassCommonFragments.h"
#include "MassCommandBuffer.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeFragments.h"
#include "MassNavigationFragments.h"
#include "MassReplicationFragments.h"
#include "MassReplicationSubsystem.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "Selection/TWSelectionVisualManager.h"
#include "DrawDebugHelpers.h"

namespace
{
	constexpr float SingleSelectionHitRadius = 100.f;

	static bool TryGetEntityWorldLocation(
		UWorld* World,
		const FMassEntityHandle& EntityHandle,
		FVector& OutLocation)
	{
		OutLocation = FVector::ZeroVector;

		if (!World)
		{
			return false;
		}

		UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
		if (!EntitySubsystem)
		{
			return false;
		}

		FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
		if (!EntityManager.IsEntityActive(EntityHandle))
		{
			return false;
		}

		if (const FTransformFragment* TransformFragment = EntityManager.GetFragmentDataPtr<FTransformFragment>(EntityHandle))
		{
			OutLocation = TransformFragment->GetTransform().GetLocation();
			return true;
		}

		return false;
	}
	
	static bool CanControlCurrentUnitSelection(const ATWPlayerController* PC)
	{
		if (!PC)
		{
			return false;
		}

		// 건물 선택 상태면 유닛 명령 불가
		if (PC->GetSelectedBuilding() != nullptr)
		{
			return false;
		}

		// 유닛이 하나도 안 선택되어 있으면 불가
		if (PC->GetLocalSelectedUnitCount() <= 0)
		{
			return false;
		}
		
		// 상대 유닛 선택 상태면 명령 불가
		const ATWPlayerState* LocalPS = PC->GetPlayerState<ATWPlayerState>();
		if (!LocalPS)
		{
			return false;
		}

		// 내 유닛 선택일 때만 허용
		return PC->GetLocalSelectedOwnerPlayerSlot() == LocalPS->PlayerSlot;
	}

	static void BuildSelectionRect2D(
		const FVector& StartLocation,
		const FVector& EndLocation,
		FVector2D& OutMin,
		FVector2D& OutMax)
	{
		OutMin.X = FMath::Min(StartLocation.X, EndLocation.X);
		OutMin.Y = FMath::Min(StartLocation.Y, EndLocation.Y);
		OutMax.X = FMath::Max(StartLocation.X, EndLocation.X);
		OutMax.Y = FMath::Max(StartLocation.Y, EndLocation.Y);
	}

	static bool IsPointInsideSelectionRect2D(
		const FVector& Point,
		const FVector2D& RectMin,
		const FVector2D& RectMax)
	{
		return
			Point.X >= RectMin.X && Point.X <= RectMax.X &&
			Point.Y >= RectMin.Y && Point.Y <= RectMax.Y;
	}

	static bool DoesBuildingOverlapSelectionRect2D(
		const ATWBaseBuilding* Building,
		const FVector2D& RectMin,
		const FVector2D& RectMax)
	{
		if (!IsValid(Building))
		{
			return false;
		}

		FVector Origin = FVector::ZeroVector;
		FVector Extent = FVector::ZeroVector;
		Building->GetActorBounds(false, Origin, Extent);

		const FVector2D BuildingMin(Origin.X - Extent.X, Origin.Y - Extent.Y);
		const FVector2D BuildingMax(Origin.X + Extent.X, Origin.Y + Extent.Y);

		const bool bOverlapX = (BuildingMin.X <= RectMax.X) && (BuildingMax.X >= RectMin.X);
		const bool bOverlapY = (BuildingMin.Y <= RectMax.Y) && (BuildingMax.Y >= RectMin.Y);
		return bOverlapX && bOverlapY;
	}
	// 디버그용
	static void DrawUnitSelectableRadiusDebug(UWorld* World, const FVector& UnitLocation, const FColor& Color)
	{
		if (!World)
		{
			return;
		}

		DrawDebugSphere(
			World,
			UnitLocation,
			SingleSelectionHitRadius,
			24,
			Color,
			false,
			0.0f,
			0,
			2.5f
		);

		DrawDebugLine(
			World,
			UnitLocation,
			UnitLocation + FVector(0.f, 0.f, 120.f),
			Color,
			false,
			0.0f,
			0,
			2.5f
		);
	}
}

ATWPlayerController::ATWPlayerController()
	: CurrentCommandType(ETWCommandType::None)
{
	PrimaryActorTick.bCanEverTick = true;
	ServerSelectedEntities.Empty();
	ClientSelectedEntities.Empty();

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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
		LocalPlayer))
	{
		if (IMC_Common)
		{
			Subsystem->AddMappingContext(IMC_Common, 0);
		}
#if WITH_EDITOR
		if (IMC_Debug)
		{
			Subsystem->AddMappingContext(IMC_Debug, -1);
		}
#endif
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	SetShowMouseCursor(false);

	InitializeSelectionVisualManager();
	RefreshSelectionVisualManager();

	InitializeUIBridge();
	RefreshUIBridge();
	RefreshDynamicMappingContexts();
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

	if (bIsLeftMousePressed && CurrentCommandType == ETWCommandType::None && !bBuildShortcutModeActive)
	{
		UpdateDragSelectionOverlay();
	}
	
	if (SelectionVisualManager)
	{
		SelectionVisualManager->Tick(DeltaSeconds);
	}
	
	// 건물
	for (TActorIterator<ATWBaseBuilding> It(GetWorld()); It; ++It)
	{
		ATWBaseBuilding* Building = *It;
		if (!IsValid(Building))
		{
			continue;
		}

		FVector Origin = FVector::ZeroVector;
		FVector Extent = FVector::ZeroVector;
		Building->GetActorBounds(false, Origin, Extent);

		DrawDebugBox(
			GetWorld(),
			Origin,
			Extent,
			FColor::Red,
			false,
			0.0f,
			0,
			2.0f
		);
	}
	
	// 유닛
	if (IsLocalController())
	{
		UMassReplicationSubsystem* RepSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
		UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
		const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();

		if (RepSubsystem && EntitySubsystem && LocalPS)
		{
			FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
			const int32 MyPlayerSlot = LocalPS->PlayerSlot;

			for (const TPair<FMassNetworkID, FMassReplicationEntityInfo>& Pair : RepSubsystem->GetEntityInfoMap())
			{
				const FMassEntityHandle EntityHandle = Pair.Value.Entity;
				if (!EntityManager.IsEntityActive(EntityHandle))
				{
					continue;
				}

				const FTransformFragment* TransformFragment =
					EntityManager.GetFragmentDataPtr<FTransformFragment>(EntityHandle);
				const FTWUnitFragment* UnitFragment =
					EntityManager.GetFragmentDataPtr<FTWUnitFragment>(EntityHandle);

				if (!TransformFragment || !UnitFragment)
				{
					continue;
				}

				const FVector UnitLocation = TransformFragment->GetTransform().GetLocation();
				const bool bIsMine = (UnitFragment->GetOwner() == MyPlayerSlot);

				DrawUnitSelectableRadiusDebug(
					GetWorld(),
					UnitLocation,
					bIsMine ? FColor::Green : FColor::Red
				);
			}
		}
	}
}

void ATWPlayerController::SetMappingContextActive(UInputMappingContext* MappingContext, int32 Priority,
	bool bShouldBeActive, bool& bCurrentActive)
{
	if (!IsLocalController() || !MappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
	if (!Subsystem)
	{
		return;
	}

	if (bShouldBeActive)
	{
		if (!bCurrentActive)
		{
			Subsystem->AddMappingContext(MappingContext, Priority);
			bCurrentActive = true;
		}
	}
	else
	{
		if (bCurrentActive)
		{
			Subsystem->RemoveMappingContext(MappingContext);
			bCurrentActive = false;
		}
	}
}

void ATWPlayerController::RefreshDynamicMappingContexts()
{
	SetMappingContextActive(IMC_UnitCommand, 1, ShouldUseUnitCommandContext(), bUnitCommandContextActive);
	SetMappingContextActive(IMC_Build, 2, ShouldUseBuildContext(), bBuildContextActive);
}

bool ATWPlayerController::ShouldUseUnitCommandContext() const
{
	if (GetSelectedBuilding() != nullptr)
	{
		return false;
	}

	if (GetLocalSelectedUnitCount() <= 0)
	{
		return false;
	}

	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return false;
	}

	return GetLocalSelectedOwnerPlayerSlot() == LocalPS->PlayerSlot;
}

bool ATWPlayerController::ShouldUseBuildContext() const
{
	return bBuildShortcutModeActive;
}

void ATWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(IsValid(LeftMouseAction));
	check(IsValid(RightMouseAction));
	
	check(IsValid(MoveCommandAction));
	check(IsValid(AttackCommandAction));
	check(IsValid(HoldCommandAction));
	
	check(IsValid(ToggleBuildModeAction));
	check(IsValid(SelectWoodCommandAction));
	check(IsValid(SelectPopulationCommandAction));
	check(IsValid(SelectStoneCommandAction));
	check(IsValid(SelectTroopCommandAction));
	check(IsValid(SelectUpgradeCommandAction));
	check(IsValid(SelectBlockingCommandAction));
	
	check(IsValid(IA_TestSpawnTroop));
	check(IsValid(IA_TestIncreasePopulation));
	check(IsValid(IA_TestDamageBlockingBuilding));
	check(IsValid(IA_TestUpgrade));
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Started, this, &ThisClass::OnStartLeftMouseAction);
	EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Completed, this, &ThisClass::OnEndLeftMouseAction);
	EnhancedInputComponent->BindAction(RightMouseAction, ETriggerEvent::Started, this, &ThisClass::OnRightMouseAction);
	
	EnhancedInputComponent->BindAction(MoveCommandAction, ETriggerEvent::Started, this, &ThisClass::OnMoveCommandAction);
	EnhancedInputComponent->BindAction(AttackCommandAction, ETriggerEvent::Started, this, &ThisClass::OnAttackCommandAction);
	EnhancedInputComponent->BindAction(HoldCommandAction, ETriggerEvent::Started, this, &ThisClass::OnHoldCommandAction);
	
	EnhancedInputComponent->BindAction(ToggleBuildModeAction, ETriggerEvent::Started, this, &ThisClass::OnToggleBuildModeAction);
	EnhancedInputComponent->BindAction(SelectWoodCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectWoodBuildingCommandAction);
	EnhancedInputComponent->BindAction(SelectPopulationCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectPopulationBuildingCommandAction);
	EnhancedInputComponent->BindAction(SelectStoneCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectStoneBuildingCommandAction);
	EnhancedInputComponent->BindAction(SelectTroopCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectTroopBuildingCommandAction);
	EnhancedInputComponent->BindAction(SelectUpgradeCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectUpgradeBuildingCommandAction);
	EnhancedInputComponent->BindAction(SelectBlockingCommandAction, ETriggerEvent::Started, this, &ThisClass::OnSelectBlockingBuildingCommandAction);
	
	EnhancedInputComponent->BindAction(IA_TestSpawnTroop, ETriggerEvent::Started, this, &ThisClass::HandleTestSpawnTroop);
	EnhancedInputComponent->BindAction(IA_TestIncreasePopulation, ETriggerEvent::Started, this, &ThisClass::HandleTestIncreasePopulation);
	EnhancedInputComponent->BindAction(IA_TestDamageBlockingBuilding, ETriggerEvent::Started, this, &ThisClass::HandleTestDamageBlockingBuilding);
	EnhancedInputComponent->BindAction(IA_TestUpgrade, ETriggerEvent::Started, this, &ThisClass::HandleTestUpgrade);
}

#pragma region 마우스
void ATWPlayerController::OnStartLeftMouseAction(const FInputActionValue& InputActionValue)
{
	bConsumeLeftMouseRelease = false;
	
	if (bBuildShortcutModeActive)
	{
		if (BuildComponent->GetBuildMode())
		{
			BuildComponent->RequestBuild();
			RefreshDynamicMappingContexts();
		}

		bConsumeLeftMouseRelease = true;
		return;
	}
	
	FHitResult HitResult;
	FVector ClickLocation;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		return;
	}
	ClickLocation = HitResult.Location;

	if (CurrentCommandType != ETWCommandType::None)
	{
		if (!CanControlCurrentUnitSelection(this))
		{
			ChangeCurrentCommandType(ETWCommandType::None);
			bConsumeLeftMouseRelease = true;
			return;
		}

		if (CurrentCommandType == ETWCommandType::Move)
		{
			ServerHandleMoveCommand(ClickLocation);
			ChangeCurrentCommandType(ETWCommandType::None);
			bConsumeLeftMouseRelease = true;
			return;
		}

		if (CurrentCommandType == ETWCommandType::Attack)
		{
			ServerHandleAttackCommand(ClickLocation);
			ChangeCurrentCommandType(ETWCommandType::None);
			bConsumeLeftMouseRelease = true;
			return;
		}

		if (CurrentCommandType == ETWCommandType::Hold)
		{
			ServerHandleHoldCommand();
			ChangeCurrentCommandType(ETWCommandType::None);
			bConsumeLeftMouseRelease = true;
			return;
		}
	}
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
	
	ClickStartLocation = ClickLocation;
}

void ATWPlayerController::OnEndLeftMouseAction(const FInputActionValue& InputActionValue)
{	
	ChangeCurrentCommandType(ETWCommandType::None);
	
	bIsLeftMousePressed = false;
	bIsDraggingSelectionVisual = false;

	if (PlayerUIBridge)
	{
		PlayerUIBridge->SetDragSelectionState(false, FVector2D::ZeroVector, FVector2D::ZeroVector);
	}
	
	if (bConsumeLeftMouseRelease)
	{
		bConsumeLeftMouseRelease = false;
		return;
	}
	
	if (bBuildShortcutModeActive)
	{
		return;
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
	if (bBuildShortcutModeActive)
	{
		if (BuildComponent->GetBuildMode())
		{
			BuildComponent->EndBuildMode();
			RefreshDynamicMappingContexts();
		}

		return;
	}
	
	// Cancle Command
	if (CurrentCommandType != ETWCommandType::None)
	{
		ChangeCurrentCommandType(ETWCommandType::None);
		return;
	}
		
	if (!CanControlCurrentUnitSelection(this))
	{
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
#pragma endregion

#pragma region 유닛 명령
inline void ATWPlayerController::OnMoveCommandAction(const FInputActionValue& InputActionValue)
{
	if (!CanControlCurrentUnitSelection(this))
	{
		return;
	}
	ChangeCurrentCommandType(ETWCommandType::Move);
}

void ATWPlayerController::OnAttackCommandAction(const FInputActionValue& InputActionValue)
{
	if (!CanControlCurrentUnitSelection(this))
	{
		return;
	}
	ChangeCurrentCommandType(ETWCommandType::Attack);
}

void ATWPlayerController::OnHoldCommandAction(const FInputActionValue& InputActionValue)
{
	if (!CanControlCurrentUnitSelection(this))
	{
		return;
	}
	ServerHandleHoldCommand();
}

void ATWPlayerController::ServerHandleMoveCommand_Implementation(const FVector& CommandLocation)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));
	if (ServerSelectedEntities.IsEmpty())
	{
		return;
	}
	
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

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
		[ThisWeakPtr, NavCommandLocation, MyPlayerSlot](FMassEntityManager& InOutEntityManager)
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
				
				const FTWUnitFragment* UnitFragment = InOutEntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
				if (!UnitFragment)
				{
					continue;
				}

				if (UnitFragment->GetOwner() != MyPlayerSlot)
				{
					continue;
				}

				ValidEntities.Add(Entity);

				if (FTWCommandTypeFragment* TypeFragment = InOutEntityManager.GetFragmentDataPtr<FTWCommandTypeFragment>(Entity))
				{
					TypeFragment->SetType(ETWMassCommand::MoveToLocation);
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
	
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	TSharedRef<FMassCommandBuffer> CommandBuffer = MakeShared<FMassCommandBuffer>();
	TWeakObjectPtr<ThisClass> ThisWeakPtr(this);

	CommandBuffer->PushCommand<FMassDeferredSetCommand>(
	[ThisWeakPtr, MyPlayerSlot](FMassEntityManager& InOutEntityManager)
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
				
				const FTWUnitFragment* UnitFragment = InOutEntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
				if (!UnitFragment)
				{
					continue;
				}

				if (UnitFragment->GetOwner() != MyPlayerSlot)
				{
					continue;
				}

				ValidEntities.Add(Entity);

				if (FTWCommandTypeFragment* TypeFragment = InOutEntityManager.GetFragmentDataPtr<FTWCommandTypeFragment>(Entity))
				{
					TypeFragment->SetType(ETWMassCommand::Hold);
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
#pragma endregion

#pragma region 선택/드래그
void ATWPlayerController::ServerHandleSingleSelect_Implementation(const FVector& CommandLocation)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));

	UWorld* World = GetWorld();
	UTWUnitSubsystem* UnitSubsystem = World ? World->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!World || !TWPS || !UnitSubsystem)
	{
		return;
	}

	auto ApplyUnitSelection =
		[this](const TArray<FMassEntityHandle>& InEntities, const int32 InSelectedOwnerPlayerSlot)
		{
			SelectedBuilding = nullptr;
			ServerSelectedEntities = InEntities;

			TArray<FMassNetworkID> NetworkIds;
			float PrimaryHealth = 0.f;
			bool bHasPrimaryHealth = false;
			BuildSelectionPayloadFromEntities(ServerSelectedEntities, NetworkIds, PrimaryHealth, bHasPrimaryHealth);
			ClientApplyUnitSelection(NetworkIds, PrimaryHealth, bHasPrimaryHealth, InSelectedOwnerPlayerSlot);
		};

	const FVector AreaMin(
		CommandLocation.X - SingleSelectionHitRadius,
		CommandLocation.Y - SingleSelectionHitRadius,
		CommandLocation.Z - 1000.f
	);

	const FVector AreaMax(
		CommandLocation.X + SingleSelectionHitRadius,
		CommandLocation.Y + SingleSelectionHitRadius,
		CommandLocation.Z + 1000.f
	);

	FMassEntityHandle BestEntity;
	float BestDistanceSq = TNumericLimits<float>::Max();
	int32 BestOwnerSlot = INDEX_NONE;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* OtherPS = PC->GetPlayerState<ATWPlayerState>();
		if (!OtherPS)
		{
			continue;
		}

		TArray<FMassEntityHandle> CandidateEntities;
		if (!UnitSubsystem->GetEntitiesInRectangle(AreaMin, AreaMax, CandidateEntities, OtherPS->PlayerSlot))
		{
			continue;
		}

		for (const FMassEntityHandle& CandidateEntity : CandidateEntities)
		{
			FVector EntityLocation = FVector::ZeroVector;
			if (!TryGetEntityWorldLocation(World, CandidateEntity, EntityLocation))
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared2D(EntityLocation, CommandLocation);
			if (DistanceSq > FMath::Square(SingleSelectionHitRadius))
			{
				continue;
			}

			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestEntity = CandidateEntity;
				BestOwnerSlot = OtherPS->PlayerSlot;
			}
		}
	}

	if (BestEntity.IsSet())
	{
		ApplyUnitSelection({ BestEntity }, BestOwnerSlot);
		return;
	}

	ServerSelectedEntities.Empty();
	SelectedBuilding = nullptr;
	ClientClearSelection();
}

void ATWPlayerController::ServerHandleMultipleSelect_Implementation(const FVector& StartLocation,
	const FVector& EndLocation)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));

	UWorld* World = GetWorld();
	UTWUnitSubsystem* UnitSubsystem = World ? World->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!World || !TWPS || !UnitSubsystem)
	{
		return;
	}

	const FVector RectCenter = (StartLocation + EndLocation) * 0.5f;
	FVector2D RectMin = FVector2D::ZeroVector;
	FVector2D RectMax = FVector2D::ZeroVector;
	BuildSelectionRect2D(StartLocation, EndLocation, RectMin, RectMax);

	auto ApplyUnitSelection =
		[this](const TArray<FMassEntityHandle>& InEntities, const int32 InSelectedOwnerPlayerSlot)
		{
			SelectedBuilding = nullptr;
			ServerSelectedEntities = InEntities;

			TArray<FMassNetworkID> NetworkIds;
			float PrimaryHealth = 0.f;
			bool bHasPrimaryHealth = false;
			BuildSelectionPayloadFromEntities(ServerSelectedEntities, NetworkIds, PrimaryHealth, bHasPrimaryHealth);
			ClientApplyUnitSelection(NetworkIds, PrimaryHealth, bHasPrimaryHealth, InSelectedOwnerPlayerSlot);
		};

	auto FindBestBuildingInRect =
		[World, &RectMin, &RectMax, &RectCenter](const bool bRequireOwnedByMe, const int32 InMyPlayerSlot)
		{
			ATWBaseBuilding* BestBuilding = nullptr;
			float BestDistanceSq = TNumericLimits<float>::Max();

			for (TActorIterator<ATWBaseBuilding> It(World); It; ++It)
			{
				ATWBaseBuilding* Building = *It;
				if (!IsValid(Building))
				{
					continue;
				}

				const bool bOwnedByMe = (Building->OwnerPlayerSlot == InMyPlayerSlot);
				if (bOwnedByMe != bRequireOwnedByMe)
				{
					continue;
				}

				if (!DoesBuildingOverlapSelectionRect2D(Building, RectMin, RectMax))
				{
					continue;
				}

				const float DistanceSq = FVector::DistSquared2D(Building->GetActorLocation(), RectCenter);
				if (DistanceSq < BestDistanceSq)
				{
					BestDistanceSq = DistanceSq;
					BestBuilding = Building;
				}
			}

			return BestBuilding;
		};
	// 내 유닛
	TArray<FMassEntityHandle> OwnedEntities;
	if (UnitSubsystem->GetEntitiesInRectangle(StartLocation, EndLocation, OwnedEntities, TWPS->PlayerSlot) && OwnedEntities.Num() > 0)
	{
		ApplyUnitSelection(OwnedEntities, TWPS->PlayerSlot);
		return;
	}

	// 내 건물 1개
	if (ATWBaseBuilding* OwnedBuilding = FindBestBuildingInRect(true, TWPS->PlayerSlot))
	{
		ServerHandleBuildingSelect(OwnedBuilding);
		return;
	}

	// 적 유닛 1개
	FMassEntityHandle BestEnemyEntity;
	float BestEnemyDistanceSq = TNumericLimits<float>::Max();
	int32 BestEnemyOwnerSlot = INDEX_NONE;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* OtherPS = PC->GetPlayerState<ATWPlayerState>();
		if (!OtherPS || OtherPS->PlayerSlot == TWPS->PlayerSlot)
		{
			continue;
		}

		TArray<FMassEntityHandle> EnemyEntities;
		if (!UnitSubsystem->GetEntitiesInRectangle(StartLocation, EndLocation, EnemyEntities, OtherPS->PlayerSlot))
		{
			continue;
		}

		for (const FMassEntityHandle& EnemyEntity : EnemyEntities)
		{
			FVector EnemyLocation = FVector::ZeroVector;
			if (!TryGetEntityWorldLocation(World, EnemyEntity, EnemyLocation))
			{
				continue;
			}

			if (!IsPointInsideSelectionRect2D(EnemyLocation, RectMin, RectMax))
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared2D(EnemyLocation, RectCenter);
			if (DistanceSq < BestEnemyDistanceSq)
			{
				BestEnemyDistanceSq = DistanceSq;
				BestEnemyEntity = EnemyEntity;
				BestEnemyOwnerSlot = OtherPS->PlayerSlot;
			}
		}
	}

	if (BestEnemyEntity.IsSet())
	{
		ApplyUnitSelection({ BestEnemyEntity }, BestEnemyOwnerSlot);
		return;
	}

	// 적 건물 1개
	if (ATWBaseBuilding* EnemyBuilding = FindBestBuildingInRect(false, TWPS->PlayerSlot))
	{
		ServerHandleBuildingSelect(EnemyBuilding);
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
#pragma endregion

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

void ATWPlayerController::ChangeCurrentCommandType(ETWCommandType CommandType)
{
	CurrentCommandType = CommandType;
	UpdateInputOverlayState();
}

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

void ATWPlayerController::InitializeSelectionVisualManager()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!SelectionVisualManager)
	{
		SelectionVisualManager = NewObject<UTWSelectionVisualManager>(this);
	}

	if (SelectionVisualManager)
	{
		SelectionVisualManager->Initialize(this);
	}
}

void ATWPlayerController::RefreshSelectionVisualManager()
{
	if (!SelectionVisualManager)
	{
		return;
	}

	if (ClientSelectedEntities.Num() > 0)
	{
		SelectionVisualManager->ApplyUnitSelection(ClientSelectedEntities);
		return;
	}

	if (SelectedBuilding)
	{
		TArray<ATWBaseBuilding*> Buildings;
		Buildings.Add(SelectedBuilding);
		SelectionVisualManager->ApplyBuildingSelection(Buildings);
		return;
	}

	SelectionVisualManager->ClearSelectionVisuals();
}

void ATWPlayerController::ClearLocalSelectionCache()
{
	if (SelectionVisualManager)
	{
		SelectionVisualManager->ClearSelectionVisuals();
	}
	
	SelectedBuilding = nullptr;
	LocalSelectedUnitCount = 0;
	LocalPrimarySelectedUnitId = NAME_None;
	bHasLocalPrimarySelectedUnitStatus = false;
	LocalSelectionSummaryItems.Empty();
	LocalSelectedOwnerPlayerSlot = INDEX_NONE;
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
	bool bInHasPrimaryHealth, int32 InSelectedOwnerPlayerSlot)
{
	ClearLocalSelectionCache();
	LocalSelectedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
	
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

	RefreshDynamicMappingContexts();
	RefreshUIBridge();
	RefreshSelectionVisualManager();
}

void ATWPlayerController::ClientApplyBuildingSelection_Implementation(ATWBaseBuilding* InBuilding)
{
	ClearLocalSelectionCache();
	SelectedBuilding = InBuilding;
	RefreshDynamicMappingContexts();
	RefreshUIBridge();
	RefreshSelectionVisualManager();
}

void ATWPlayerController::ClientClearSelection_Implementation()
{
	ClearLocalSelectionCache();
	RefreshDynamicMappingContexts();
	RefreshUIBridge();
	RefreshSelectionVisualManager();
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
		ChangeCurrentCommandType(ETWCommandType::Move);
		UE_LOG(LogTemp, Log, TEXT("[UI Click] Move command armed"));
		return;

	case ETWCommandType::Attack:
		ChangeCurrentCommandType(ETWCommandType::Attack);
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
	
	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI Produce] Server ignored: PlayerState is invalid"));
		return;
	}
	
	if (CurrentSelectedBuilding->OwnerPlayerSlot != TWPS->PlayerSlot)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[UI Produce] Rejected: enemy building selected / Building=%s / MySlot=%d / OwnerSlot=%d"),
			*CurrentSelectedBuilding->GetName(),
			TWPS->PlayerSlot,
			CurrentSelectedBuilding->OwnerPlayerSlot
		);
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
void ATWPlayerController::OnToggleBuildModeAction()
{
	bBuildShortcutModeActive = !bBuildShortcutModeActive;

	if (!bBuildShortcutModeActive)
	{
		if (BuildComponent->GetBuildMode())
		{
			BuildComponent->EndBuildMode();
		}

		ChangeCurrentCommandType(ETWCommandType::None);
	}
	UE_LOG(LogTemp, Warning, TEXT("건설 모드: %s"), bBuildShortcutModeActive ? TEXT("ON") : TEXT("OFF"));

	RefreshDynamicMappingContexts();
}

void ATWPlayerController::OnSelectWoodBuildingCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->SelectBuildingToConstruct(EBuildingCategory::WoodResourceProduction);
	}
}

void ATWPlayerController::OnSelectStoneBuildingCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->SelectBuildingToConstruct(EBuildingCategory::StoneResourceProduction);
	}
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
	if (BuildComponent)
	{
		BuildComponent->SelectBuildingToConstruct(EBuildingCategory::TroopProduction);
	}
}

void ATWPlayerController::OnSelectUpgradeBuildingCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->SelectBuildingToConstruct(EBuildingCategory::Upgrade);
	}
}

void ATWPlayerController::OnSelectBlockingBuildingCommandAction()
{
	if (BuildComponent)
	{
		BuildComponent->SelectBuildingToConstruct(EBuildingCategory::Blocking);
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

FName ATWPlayerController::ConvertCommandTypeToCommandId(ETWCommandType InCommandType) const
{
	switch (InCommandType)
	{
	case ETWCommandType::Move:
		return TEXT("Move");

	case ETWCommandType::Attack:
		return TEXT("Attack");

	default:
		return NAME_None;
	}
}

#pragma endregion

