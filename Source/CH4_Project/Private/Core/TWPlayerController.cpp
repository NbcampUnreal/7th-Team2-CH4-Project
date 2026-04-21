#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"

#include "Building/TWTroopSpawnBuilding.h"
#include "Building/TWPopulationBuilding.h"
#include "Building/TWNexusBuilding.h"
#include "Building/TWBaseBuilding.h"
#include "Building/TWUpgradeBuilding.h"
#include "Building/TWBlockingBuilding.h"
#include "Building/TWResourceBuilding.h"

#include "UI/Data/TWUIDataTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Blueprint/UserWidget.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"

#include "NavigationSystem.h"
#include "Component/TWBuildComponent.h"
#include "Component/TWPlayerUIControllerComponent.h"
#include "Component/TWPlayerSelectionVisualComponent.h"
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
#include "DrawDebugHelpers.h"
#include "HeroUnit/TWHeroUnitBase.h"

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

		if (PC->GetSelectedBuilding() != nullptr)
		{
			return false;
		}

		if (PC->GetLocalSelectedUnitCount() <= 0)
		{
			return false;
		}

		const ATWPlayerState* LocalPS = PC->GetPlayerState<ATWPlayerState>();
		if (!LocalPS)
		{
			return false;
		}

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

	static bool IsMouseOverHitTestableUI()
	{
		if (!FSlateApplication::IsInitialized())
		{
			return false;
		}

		FSlateApplication& SlateApp = FSlateApplication::Get();

		const FWidgetPath WidgetPath = SlateApp.LocateWindowUnderMouse(
			SlateApp.GetCursorPos(),
			SlateApp.GetInteractiveTopLevelWindows(),
			true
		);

		if (!WidgetPath.IsValid())
		{
			return false;
		}

		for (int32 Index = WidgetPath.Widgets.Num() - 1; Index >= 0; --Index)
		{
			const TSharedRef<SWidget>& Widget = WidgetPath.Widgets[Index].Widget;
			const FString WidgetType = Widget->GetTypeAsString();

			if (
				WidgetType == TEXT("SViewport") ||
				WidgetType == TEXT("SPIEViewport") ||
				WidgetType == TEXT("SGameLayerManager")
			)
			{
				return false;
			}

			if (Widget->IsEnabled())
			{
				return true;
			}
		}

		return false;
	}
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
{
	PrimaryActorTick.bCanEverTick = true;
	ServerSelectedEntities.Empty();
	ClientSelectedEntities.Empty();
	ArmedCommandId = NAME_None;

	BuildComponent = CreateDefaultSubobject<UTWBuildComponent>(TEXT("BuildComponent"));
	PlayerUIControllerComponent = CreateDefaultSubobject<UTWPlayerUIControllerComponent>(TEXT("PlayerUIControllerComponent"));
	PlayerSelectionVisualComponent = CreateDefaultSubobject<UTWPlayerSelectionVisualComponent>(TEXT("PlayerSelectionVisualComponent"));
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

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
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
		if (PlayerUIControllerComponent)
		{
			PlayerUIControllerComponent->UpdateCursorOverlayPosition(
				LastValidMouseScreenPosition,
				bIsEdgeScrollingNow
			);
		}

		bWasEdgeScrollingLastFrame = bIsEdgeScrollingNow;
	}

	if (bIsLeftMousePressed && ArmedCommandId.IsNone() && (!BuildComponent || !BuildComponent->GetBuildMode()))
	{
		UpdateDragSelectionOverlay();
	}

	if (PlayerSelectionVisualComponent)
	{
		PlayerSelectionVisualComponent->TickVisuals(DeltaSeconds);
	}

	RefreshLocalSelectionRuntimeData();
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
	SetMappingContextActive(IMC_BuildingCommand, 1, ShouldUseBuildingCommandContext(), bBuildingCommandContextActive);
	SetMappingContextActive(IMC_Build, 2, ShouldUseBuildContext(), bBuildContextActive);
}

bool ATWPlayerController::ShouldUseUnitCommandContext() const
{
	if (bBuildShortcutModeActive)
	{
		return false;
	}

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

bool ATWPlayerController::ShouldUseBuildingCommandContext() const
{
	if (bBuildShortcutModeActive)
	{
		return false;
	}
	
	if (BuildComponent && BuildComponent->GetBuildMode())
	{
		return false;
	}

	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	ATWBaseBuilding* TargetBuilding  = GetSelectedBuilding();

	if (!LocalPS || !IsValid(TargetBuilding ))
	{
		return false;
	}
	
	if (TargetBuilding ->OwnerPlayerSlot != LocalPS->PlayerSlot)
	{
		return false;
	}

	return
		(Cast<ATWTroopSpawnBuilding>(TargetBuilding) != nullptr) ||
		(Cast<ATWPopulationBuilding>(TargetBuilding) != nullptr) ||
		(Cast<ATWUpgradeBuilding>(TargetBuilding) != nullptr);
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

	check(IsValid(QueueHotkeyQAction));
	check(IsValid(QueueHotkeyWAction));
	check(IsValid(QueueHotkeyEAction));
	check(IsValid(QueueHotkeyAAction));
	
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

	EnhancedInputComponent->BindAction(QueueHotkeyQAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyQ);
	EnhancedInputComponent->BindAction(QueueHotkeyWAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyW);
	EnhancedInputComponent->BindAction(QueueHotkeyEAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyE);
	EnhancedInputComponent->BindAction(QueueHotkeyAAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyA);
	
	EnhancedInputComponent->BindAction(IA_TestDamageBlockingBuilding, ETriggerEvent::Started, this, &ThisClass::HandleTestDamageBlockingBuilding);
	EnhancedInputComponent->BindAction(IA_TestUpgrade, ETriggerEvent::Started, this, &ThisClass::HandleTestUpgrade);
	
	EnhancedInputComponent->BindAction(IA_Menu, ETriggerEvent::Started, this, &ATWPlayerController::ToggleMenu);
}

#pragma region 마우스
void ATWPlayerController::OnStartLeftMouseAction(const FInputActionValue& InputActionValue)
{
	bConsumeLeftMouseRelease = false;

	const bool bMouseOverUI = IsMouseOverHitTestableUI();
	if (bMouseOverUI)
	{
		bIsLeftMousePressed = false;
		bIsDraggingSelectionVisual = false;
		bConsumeLeftMouseRelease = true;

		if (PlayerUIControllerComponent)
		{
			PlayerUIControllerComponent->UpdateDragSelectionOverlay(
				false,
				FVector2D::ZeroVector,
				FVector2D::ZeroVector
			);
		}
		return;
	}

	if (BuildComponent && BuildComponent->GetBuildMode())
	{
		BuildComponent->RequestBuild();
		RefreshDynamicMappingContexts();
		bConsumeLeftMouseRelease = true;
		return;
	}

	FHitResult HitResult;
	FVector ClickLocation = FVector::ZeroVector;

	const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, HitResult);
	if (!bHit)
	{
		return;
	}

	ClickLocation = HitResult.Location;

	if (!ArmedCommandId.IsNone())
	{
		if (!CanControlCurrentUnitSelection(this))
		{
			ClearArmedCommandId();
			bConsumeLeftMouseRelease = true;
			return;
		}

		const FUICommandMetaRow* ArmedMeta = FindCommandMetaRowFromTable(ArmedCommandId);
		if (!ArmedMeta)
		{
			ClearArmedCommandId();
			bConsumeLeftMouseRelease = true;
			return;
		}

		if (ArmedMeta->CommandType == ETWCommandType::Move)
		{
			ServerHandleMoveCommand(ClickLocation);
			ClearArmedCommandId();
			bConsumeLeftMouseRelease = true;
			return;
		}

		if (ArmedMeta->CommandType == ETWCommandType::Attack)
		{
			ServerHandleAttackCommand(ClickLocation);
			ClearArmedCommandId();
			bConsumeLeftMouseRelease = true;
			return;
		}

		if (ArmedMeta->CommandType == ETWCommandType::Hold)
		{
			ServerHandleHoldCommand();
			ClearArmedCommandId();
			bConsumeLeftMouseRelease = true;
			return;
		}
	}

	bIsLeftMousePressed = true;

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
	ClearArmedCommandId();

	bIsLeftMousePressed = false;
	bIsDraggingSelectionVisual = false;

	if (PlayerUIControllerComponent)
	{
		PlayerUIControllerComponent->UpdateDragSelectionOverlay(
			false,
			FVector2D::ZeroVector,
			FVector2D::ZeroVector
		);
	}

	if (bConsumeLeftMouseRelease)
	{
		bConsumeLeftMouseRelease = false;
		return;
	}

	if (BuildComponent && BuildComponent->GetBuildMode())
	{
		return;
	}

	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		return;
	}

	const FVector ClickLocation = HitResult.Location;

	if (FVector::DistSquared(ClickLocation, ClickStartLocation) < 50.0f * 50.0f)
	{
		if (ATWBaseBuilding* ClickedBuilding = Cast<ATWBaseBuilding>(HitResult.GetActor()))
		{
			ServerHandleBuildingSelect(ClickedBuilding);
			return;
		}

		ServerHandleSingleSelect(ClickLocation);
	}
	else
	{
		ServerHandleMultipleSelect(ClickStartLocation, ClickLocation);
	}
}

void ATWPlayerController::OnRightMouseAction(const FInputActionValue& InputActionValue)
{
	if (IsMouseOverHitTestableUI())
	{
		return;
	}

	if (bBuildShortcutModeActive)
	{
		if (BuildComponent && BuildComponent->GetBuildMode())
		{
			BuildComponent->EndBuildMode();
			RefreshDynamicMappingContexts();
		}

		return;
	}

	if (!ArmedCommandId.IsNone())
	{
		ClearArmedCommandId();
		return;
	}

	if (!CanControlCurrentUnitSelection(this))
	{
		return;
	}

	FHitResult HitResult;
	if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
	{
		const FVector ClickLocation = HitResult.Location;
		ServerHandleMoveCommand(ClickLocation);
	}
}
#pragma endregion

#pragma region 유닛 명령
void ATWPlayerController::OnMoveCommandAction(const FInputActionValue& InputActionValue)
{
	HandleCommandById(TEXT("Move"));
}

void ATWPlayerController::OnAttackCommandAction(const FInputActionValue& InputActionValue)
{
	HandleCommandById(TEXT("Attack"));
}

void ATWPlayerController::OnHoldCommandAction(const FInputActionValue& InputActionValue)
{
	HandleCommandById(TEXT("Hold"));
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
	if (NavSys && NavSys->ProjectPointToNavigation(CommandLocation, ProjectedLocation, FVector(500.f, 500.f, 500.f)))
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
			UTWUnitSubsystem* UnitSubsystem = ThisWeakPtr->GetWorld()->GetSubsystem<UTWUnitSubsystem>();
			if (false == IsValid(UnitSubsystem))
			{
				return;
			}

			ETWMassCommand CommandType = ETWMassCommand::None;
			FVector TargetLocation = FVector::ZeroVector;
			FMassEntityHandle TargetEntity;
			if (UnitSubsystem->FindNearestAnyEntity(NavCommandLocation, TargetEntity))
			{
				CommandType = ETWMassCommand::MoveToTarget;
			}
			else
			{
				CommandType = ETWMassCommand::MoveToLocation;
				TargetLocation = NavCommandLocation;
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

				if (FTWCommandFragment* CommandFragment = InOutEntityManager.GetFragmentDataPtr<FTWCommandFragment>(Entity))
				{
					CommandFragment->SetType(CommandType);
					CommandFragment->SetLocation(TargetLocation);
					CommandFragment->SetTarget(TargetEntity);
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
					SignalSubsystem->SignalEntities(CommandSignal, ValidEntities);
				}
			}
		});

	EntityManager.AppendCommands(CommandBuffer);

	UE_LOG(LogTemp, Log, TEXT("[Move] SelectedEntities=%d / Target=%s"),
	       ServerSelectedEntities.Num(),
	       *NavCommandLocation.ToString());
}

bool ATWPlayerController::TryFindAttackableBuildingCandidate(
	const FVector& CommandLocation,
	int32 MyPlayerSlot,
	ATWBaseBuilding*& OutTargetBuilding,
	FVector& OutResolvedTargetLocation
)
{
	OutTargetBuilding = nullptr;
	OutResolvedTargetLocation = CommandLocation;

	UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return false;
	}

	FTWAttackableBuildingCandidate Candidate;
	if (!UnitSubsystem->FindNearestEnemyBuildingCandidate(CommandLocation, MyPlayerSlot, Candidate, 200.0f))
	{
		return false;
	}

	OutTargetBuilding = Candidate.Building.Get();
	OutResolvedTargetLocation = Candidate.TargetLocation;
	return IsValid(OutTargetBuilding);
}

bool ATWPlayerController::TryResolveAttackableTargetPreview(
	const FVector& CommandLocation,
	int32 MyPlayerSlot,
	FMassEntityHandle& OutTargetEntity,
	ATWBaseBuilding*& OutTargetBuilding,
	FVector& OutResolvedTargetLocation
)
{
	OutTargetEntity = FMassEntityHandle();
	OutTargetBuilding = nullptr;
	OutResolvedTargetLocation = CommandLocation;

	UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return false;
	}
	
	if (UnitSubsystem->FindNearestEnemyEntity(CommandLocation, OutTargetEntity, MyPlayerSlot, 100.0f))
	{
		OutResolvedTargetLocation = CommandLocation;
		return true;
	}
	
	if (TryFindAttackableBuildingCandidate(CommandLocation, MyPlayerSlot, OutTargetBuilding, OutResolvedTargetLocation))
	{
		return true;
	}

	return false;
}

void ATWPlayerController::ServerHandleAttackCommand_Implementation(const FVector& CommandLocation)
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
	if (NavSys && NavSys->ProjectPointToNavigation(CommandLocation, ProjectedLocation, FVector(500.f, 500.f, 500.f)))
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
			UTWUnitSubsystem* UnitSubsystem = ThisWeakPtr->GetWorld()->GetSubsystem<UTWUnitSubsystem>();
			if (false == IsValid(UnitSubsystem))
			{
				return;
			}

			ETWMassCommand CommandType = ETWMassCommand::AttackToLocation;
			FVector TargetLocation = NavCommandLocation;
			FMassEntityHandle TargetEntity;
			ATWBaseBuilding* TargetBuilding = nullptr;

			const bool bHasAnyAttackableTarget =
				ThisWeakPtr->TryResolveAttackableTargetPreview(
					NavCommandLocation,
					MyPlayerSlot,
					TargetEntity,
					TargetBuilding,
					TargetLocation
				);

			if (bHasAnyAttackableTarget)
			{
				if (TargetEntity.IsSet())
				{
					CommandType = ETWMassCommand::AttackToTarget;
				}
				else if (IsValid(TargetBuilding))
				{
					CommandType = ETWMassCommand::AttackToTarget;

					UE_LOG(
						LogTemp,
						Log,
						TEXT("[AttackPreview] Building target candidate found: %s / Location=%s"),
						*GetNameSafe(TargetBuilding),
						*TargetLocation.ToString()
					);
				}
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

				if (FTWCommandFragment* CommandFragment = InOutEntityManager.GetFragmentDataPtr<FTWCommandFragment>(Entity))
				{
					CommandFragment->SetType(CommandType);
					CommandFragment->SetLocation(TargetLocation);
					CommandFragment->SetTarget(TargetEntity);
					CommandFragment->SetTargetBuilding(TargetBuilding);
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
					SignalSubsystem->SignalEntities(CommandSignal, ValidEntities);
				}
			}
		});

	EntityManager.AppendCommands(CommandBuffer);

	UE_LOG(LogTemp, Log, TEXT("[Attack] SelectedEntities=%d / Target=%s"),
	       ServerSelectedEntities.Num(),
	       *NavCommandLocation.ToString());
}

void ATWPlayerController::ServerHandleHoldCommand_Implementation()
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

				if (FTWCommandFragment* TypeFragment = InOutEntityManager.GetFragmentDataPtr<FTWCommandFragment>(Entity))
				{
					TypeFragment->SetType(ETWMassCommand::Hold);
				}
			}

			if (UMassSignalSubsystem* SignalSubsystem = InOutEntityManager.GetWorld()->GetSubsystem<UMassSignalSubsystem>())
			{
				if (ValidEntities.Num() > 0)
				{
					SignalSubsystem->SignalEntities(CommandSignal, ValidEntities);
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

	TArray<FMassEntityHandle> OwnedEntities;
	if (UnitSubsystem->GetEntitiesInRectangle(StartLocation, EndLocation, OwnedEntities, TWPS->PlayerSlot) && OwnedEntities.Num() > 0)
	{
		ApplyUnitSelection(OwnedEntities, TWPS->PlayerSlot);
		return;
	}

	if (ATWBaseBuilding* OwnedBuilding = FindBestBuildingInRect(true, TWPS->PlayerSlot))
	{
		ServerHandleBuildingSelect(OwnedBuilding);
		return;
	}

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

	APawn* MyPawn = GetPawn();
	if (!IsValid(MyPawn))
	{
		return false;
	}

	const FVector2D& MousePosition = CurrentFrameRawMousePosition;

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	FVector MoveDirection = FVector::ZeroVector;

	if (MousePosition.X <= ScreenEdgeEnterMargin)
	{
		MoveDirection.Y = -1.f;
	}
	else if (MousePosition.X >= static_cast<float>(ViewportSizeX) - ScreenEdgeEnterMargin)
	{
		MoveDirection.Y = 1.f;
	}

	if (MousePosition.Y <= ScreenEdgeEnterMargin)
	{
		MoveDirection.X = 1.f;
	}
	else if (MousePosition.Y >= static_cast<float>(ViewportSizeY) - ScreenEdgeEnterMargin)
	{
		MoveDirection.X = -1.f;
	}

	FVector PawnMoveDirection =
		MoveDirection.Y * MyPawn->GetActorTransform().GetRotation().GetRightVector() +
		MoveDirection.X * MyPawn->GetActorTransform().GetRotation().GetForwardVector();

	PawnMoveDirection.Z = 0.0f;

	const bool bIsEdgeScrollingNow = !PawnMoveDirection.IsNearlyZero();

	if (bIsEdgeScrollingNow)
	{
		MyPawn->AddMovementInput(PawnMoveDirection.GetSafeNormal(), ScrollSpeed * DeltaSeconds);
	}

	return bIsEdgeScrollingNow;
}
#pragma region 병력 스폰
void ATWPlayerController::OnQueueHotkeyQ(const FInputActionValue&)
{
	HandleBuildingProductionSlot(0);
}

void ATWPlayerController::OnQueueHotkeyW(const FInputActionValue&)
{
	HandleBuildingProductionSlot(1);
}

void ATWPlayerController::OnQueueHotkeyE(const FInputActionValue&)
{
	HandleBuildingProductionSlot(2);
}

void ATWPlayerController::OnQueueHotkeyA(const FInputActionValue&)
{
	HandleBuildingProductionSlot(3);
}
void ATWPlayerController::HandleBuildingProductionSlot(int32 SlotIndex)
{
	if (!ShouldUseBuildingCommandContext())
	{
		return;
	}

	ATWBaseBuilding* TargetBuilding = GetSelectedBuilding();
	if (!IsValid(TargetBuilding))
	{
		return;
	}

	FName ResolvedCommandId = NAME_None;

	if (PlayerUIControllerComponent &&
		PlayerUIControllerComponent->TryGetVisibleCommandIdAtIndex(SlotIndex, ResolvedCommandId))
	{
		HandleCommandById(ResolvedCommandId);
	}
}
#pragma endregion

#pragma region UI
void ATWPlayerController::InitializeUIBridge()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!PlayerUIControllerComponent)
	{
		return;
	}

	PlayerUIControllerComponent->InitializeUI(
		this,
		HUDRootWidgetClass,
		CommandMetaTable,
		SelectionPresentationTable,
		bUseUIDebugFallback,
		DefaultSelectedUnitId,
		DefaultSelectedBuildingId,
		DefaultMultiSelectedUnitId
	);
}

void ATWPlayerController::RefreshUIBridge()
{
	if (PlayerUIControllerComponent)
	{
		PlayerUIControllerComponent->RefreshUI();
	}
}

ATWHeroUnitBase* ATWPlayerController::GetOwnedHeroUnit() const
{
	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return nullptr;
	}

	for (TActorIterator<ATWHeroUnitBase> It(GetWorld()); It; ++It)
	{
		ATWHeroUnitBase* Hero = *It;
		if (!IsValid(Hero))
		{
			continue;
		}

		if (Hero->GetOwnerPlayerSlot() == LocalPS->PlayerSlot)
		{
			return Hero;
		}
	}

	return nullptr;
}

bool ATWPlayerController::IsOwnedHeroCurrentlySelected() const
{
	if (!IsLocalController())
	{
		return false;
	}

	if (ClientSelectedEntities.IsEmpty())
	{
		return false;
	}

	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return false;
	}

	const UTWUnitSubsystem* UnitSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	if (!UnitSubsystem)
	{
		return false;
	}

	for (const FMassNetworkID& NetId : ClientSelectedEntities)
	{
		int32 OwnerPlayerSlot = INDEX_NONE;
		if (!UnitSubsystem->TryGetUnitOwnerPlayerSlot(NetId, OwnerPlayerSlot))
		{
			continue;
		}
		
		if (OwnerPlayerSlot != LocalPS->PlayerSlot)
		{
			continue;
		}

		FName UnitId = NAME_None;
		if (!UnitSubsystem->TryGetUnitID(NetId, UnitId))
		{
			continue;
		}
		
		const FString UnitIdString = UnitId.ToString();
		if (UnitIdString.Contains(TEXT("Hero")))
		{
			return true;
		}
	}

	return false;
}

void ATWPlayerController::InitializeSelectionVisualManager()
{
	if (!IsLocalController())
	{
		return;
	}

	if (PlayerSelectionVisualComponent)
	{
		PlayerSelectionVisualComponent->InitializeVisuals(this);
	}
}

void ATWPlayerController::RefreshSelectionVisualManager()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!PlayerSelectionVisualComponent)
	{
		return;
	}

	PlayerSelectionVisualComponent->RefreshSelectionVisuals(
		ClientSelectedEntities,
		SelectedBuilding,
		LocalSelectedOwnerPlayerSlot
	);
}

void ATWPlayerController::NotifyRecentCombatUnitDamaged(
	const FMassNetworkID& InUnitNetId,
	int32 InOwnerPlayerSlot,
	float InVisibleTime
)
{
	if (!IsLocalController())
	{
		return;
	}

	if (!PlayerSelectionVisualComponent)
	{
		return;
	}

	PlayerSelectionVisualComponent->NotifyRecentCombatUnitDamaged(
		InUnitNetId,
		InOwnerPlayerSlot,
		InVisibleTime
	);
}

void ATWPlayerController::ClientNotifyRecentCombatUnitDamaged_Implementation(
	const FMassNetworkID& InUnitNetId,
	int32 InOwnerPlayerSlot,
	float InVisibleTime
)
{
	NotifyRecentCombatUnitDamaged(
		InUnitNetId,
		InOwnerPlayerSlot,
		InVisibleTime
	);
}

void ATWPlayerController::NotifyRecentCombatBuildingDamaged(
	ATWBaseBuilding* InBuilding,
	float InVisibleTime
)
{
	if (!IsLocalController())
	{
		return;
	}

	if (!PlayerSelectionVisualComponent)
	{
		return;
	}

	PlayerSelectionVisualComponent->NotifyRecentCombatBuildingDamaged(
		InBuilding,
		InVisibleTime
	);
}

void ATWPlayerController::ClientNotifyRecentCombatBuildingDamaged_Implementation(
	ATWBaseBuilding* InBuilding,
	float InVisibleTime
)
{
	NotifyRecentCombatBuildingDamaged(
		InBuilding,
		InVisibleTime
	);
}

void ATWPlayerController::ClearLocalSelectionCache()
{
	SelectedBuilding = nullptr;
	LocalSelectedUnitCount = 0;
	LocalPrimarySelectedUnitId = NAME_None;
	LocalPrimarySelectedUnitStatus = FTWUnitStatus();
	bHasLocalPrimarySelectedUnitStatus = false;
	LocalSelectionSummaryItems.Empty();
	LocalSelectedOwnerPlayerSlot = INDEX_NONE;
	ClientSelectedEntities.Empty();

	if (PlayerSelectionVisualComponent)
	{
		PlayerSelectionVisualComponent->ClearSelectionVisuals();
	}
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

void ATWPlayerController::RefreshLocalSelectionRuntimeData()
{
	if (!IsLocalController())
	{
		return;
	}

	if (SelectedBuilding)
	{
		return;
	}

	if (ClientSelectedEntities.IsEmpty())
	{
		LocalPrimarySelectedUnitStatus = FTWUnitStatus();
		bHasLocalPrimarySelectedUnitStatus = false;
		return;
	}

	RefreshLocalPrimarySelectedUnitStatus();
}

bool ATWPlayerController::RefreshLocalPrimarySelectedUnitStatus()
{
	if (ClientSelectedEntities.IsEmpty())
	{
		LocalPrimarySelectedUnitStatus = FTWUnitStatus();
		bHasLocalPrimarySelectedUnitStatus = false;
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UMassReplicationSubsystem* RepSubsystem = World->GetSubsystem<UMassReplicationSubsystem>();
	if (!RepSubsystem)
	{
		return false;
	}

	const FMassReplicationEntityInfo* EntityInfo = RepSubsystem->GetEntityInfoMap().Find(ClientSelectedEntities[0]);
	if (!EntityInfo)
	{
		LocalPrimarySelectedUnitStatus = FTWUnitStatus();
		bHasLocalPrimarySelectedUnitStatus = false;
		return false;
	}

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	if (!EntityManager.IsEntityActive(EntityInfo->Entity))
	{
		LocalPrimarySelectedUnitStatus = FTWUnitStatus();
		bHasLocalPrimarySelectedUnitStatus = false;
		return false;
	}

	if (FTWStatusFragment* StatusFragment = EntityManager.GetFragmentDataPtr<FTWStatusFragment>(EntityInfo->Entity))
	{
		LocalPrimarySelectedUnitStatus = StatusFragment->GetMutableStatus();
		bHasLocalPrimarySelectedUnitStatus = true;
		return true;
	}

	LocalPrimarySelectedUnitStatus = FTWUnitStatus();
	bHasLocalPrimarySelectedUnitStatus = false;
	return false;
}

void ATWPlayerController::ClientApplyUnitSelection_Implementation(
	const TArray<FMassNetworkID>& InNetworkIds,
	float InPrimaryHealth,
	bool bInHasPrimaryHealth,
	int32 InSelectedOwnerPlayerSlot)
{
	ClearLocalSelectionCache();

	LocalSelectedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
	LocalSelectedUnitCount = InNetworkIds.Num();
	ClientSelectedEntities = InNetworkIds;

	UMassReplicationSubsystem* RepSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
	if (RepSubsystem)
	{
		FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
		TArray<FName> UnitIds;

		for (const FMassNetworkID& NetID : InNetworkIds)
		{
			const FMassReplicationEntityInfo* EntityInfo = RepSubsystem->GetEntityInfoMap().Find(NetID);
			if (!EntityInfo)
			{
				continue;
			}

			const FMassEntityHandle EntityHandle = EntityInfo->Entity;
			const FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(EntityHandle);
			if (!UnitFragment)
			{
				continue;
			}

			UnitIds.Add(UnitFragment->GetUnitID());
		}

		if (UnitIds.Num() > 0)
		{
			LocalPrimarySelectedUnitId = UnitIds[0];
			RebuildLocalSelectionSummary(UnitIds);
		}
	}

	LocalPrimarySelectedUnitStatus = FTWUnitStatus();
	bHasLocalPrimarySelectedUnitStatus = false;

	if (!RefreshLocalPrimarySelectedUnitStatus() && bInHasPrimaryHealth)
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

	if (SelectedBuilding)
	{
		LocalSelectedOwnerPlayerSlot = SelectedBuilding->OwnerPlayerSlot;
	}

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
	if (PlayerUIControllerComponent)
	{
		PlayerUIControllerComponent->ForceRefreshSelectionBridge();
	}
	else
	{
		RefreshUIBridge();
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

bool ATWPlayerController::CanExecuteCommand(const FUICommandMetaRow* CommandMeta, FName CommandId) const
{
	if (!CommandMeta)
	{
		return false;
	}

	switch (CommandMeta->CommandType)
	{
	case ETWCommandType::Move:
	case ETWCommandType::Attack:
	case ETWCommandType::Hold:
		{
			return CanControlCurrentUnitSelection(this);
		}

	case ETWCommandType::BuildStructure:
		{
			// 건설은 빌드 단축키 모드에서만 받도록 제한
			return bBuildShortcutModeActive;
		}

	case ETWCommandType::ProduceUnit:
		{
			ATWBaseBuilding* Selected = GetSelectedBuilding();
			if (!IsValid(Selected))
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Produce blocked: no selected building | CommandId=%s"),
					*CommandId.ToString()
				);
				return false;
			}

			const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
			if (!LocalPS)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Produce blocked: no local player state | CommandId=%s"),
					*CommandId.ToString()
				);
				return false;
			}

			if (Selected->OwnerPlayerSlot != LocalPS->PlayerSlot)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Produce blocked: selected building is not mine | CommandId=%s | BuildingOwner=%d | MySlot=%d"),
					*CommandId.ToString(),
					Selected->OwnerPlayerSlot,
					LocalPS->PlayerSlot
				);
				return false;
			}

			// 인구 증가 전용
			if (CommandId == TEXT("IncreasePopulation"))
			{
				const bool bValidPopulationBuilding = (Cast<ATWPopulationBuilding>(Selected) != nullptr);
				if (!bValidPopulationBuilding)
				{
					UE_LOG(
						LogTemp,
						Warning,
						TEXT("[Command] Produce blocked: IncreasePopulation requires ATWPopulationBuilding")
					);
				}
				return bValidPopulationBuilding;
			}

			// 일반 병력 생산 전용
			const bool bValidTroopBuilding = (Cast<ATWTroopSpawnBuilding>(Selected) != nullptr);
			if (!bValidTroopBuilding)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Produce blocked: troop production requires ATWTroopSpawnBuilding | CommandId=%s"),
					*CommandId.ToString()
				);
			}
			return bValidTroopBuilding;
		}

	case ETWCommandType::Research:
		{
			ATWBaseBuilding* Selected = GetSelectedBuilding();
			if (!IsValid(Selected))
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Research blocked: no selected building | CommandId=%s"),
					*CommandId.ToString()
				);
				return false;
			}

			const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
			if (!LocalPS)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Research blocked: no local player state | CommandId=%s"),
					*CommandId.ToString()
				);
				return false;
			}

			if (Selected->OwnerPlayerSlot != LocalPS->PlayerSlot)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Research blocked: selected building is not mine | CommandId=%s | BuildingOwner=%d | MySlot=%d"),
					*CommandId.ToString(),
					Selected->OwnerPlayerSlot,
					LocalPS->PlayerSlot
				);
				return false;
			}

			const bool bValidUpgradeBuilding = (Cast<ATWUpgradeBuilding>(Selected) != nullptr);
			if (!bValidUpgradeBuilding)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Research blocked: selected building is not ATWUpgradeBuilding | CommandId=%s"),
					*CommandId.ToString()
				);
			}
			return bValidUpgradeBuilding;
		}

	default:
		return true;
	}
}
bool ATWPlayerController::TryHandleImmediateCommand(const FUICommandMetaRow* CommandMeta, FName CommandId)
{
	if (!CommandMeta)
	{
		return false;
	}

	if (CommandMeta->CommandType == ETWCommandType::OpenContext)
	{
		if (CommandId == TEXT("BuildMenu"))
		{
			bBuildShortcutModeActive = !bBuildShortcutModeActive;

			if (!bBuildShortcutModeActive)
			{
				if (BuildComponent && BuildComponent->GetBuildMode())
				{
					BuildComponent->EndBuildMode();
				}

				ClearArmedCommandId();
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("건설 모드: %s"),
				bBuildShortcutModeActive ? TEXT("ON") : TEXT("OFF")
			);

			if (PlayerUIControllerComponent)
			{
				PlayerUIControllerComponent->RefreshBuildModeNotification();
			}

			RefreshDynamicMappingContexts();
			RefreshUIBridge();
			return true;
		}
	}

	if (CommandMeta->CommandType == ETWCommandType::Back)
	{
		if (BuildComponent && BuildComponent->GetBuildMode())
		{
			BuildComponent->EndBuildMode();
		}

		bBuildShortcutModeActive = false;
		ClearArmedCommandId();

		if (PlayerUIControllerComponent)
		{
			PlayerUIControllerComponent->RefreshBuildModeNotification();
		}

		RefreshDynamicMappingContexts();
		RefreshUIBridge();
		return true;
	}

	return false;
}

bool ATWPlayerController::TryHandleArmedCommand(const FUICommandMetaRow* CommandMeta, FName CommandId)
{
	if (!CommandMeta)
	{
		return false;
	}

	switch (CommandMeta->CommandType)
	{
	case ETWCommandType::Move:
	case ETWCommandType::Attack:
		if (!CanControlCurrentUnitSelection(this))
		{
			return true;
		}

		SetArmedCommandId(CommandId);
		return true;

	case ETWCommandType::Hold:
		if (!CanControlCurrentUnitSelection(this))
		{
			return true;
		}

		ServerHandleHoldCommand();
		ClearArmedCommandId();
		return true;

	default:
		return false;
	}
}

bool ATWPlayerController::TryHandleServerRoutedCommand(const FUICommandMetaRow* CommandMeta, FName CommandId)
{
	if (!CommandMeta)
	{
		return false;
	}

	switch (CommandMeta->CommandType)
	{
	case ETWCommandType::BuildStructure:
		{
			EBuildingCategory Category = EBuildingCategory::None;
			if (!TryResolveBuildingCategoryFromPayload(CommandMeta->PayloadId, Category))
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command] Build payload resolve failed: %s"),
					*CommandMeta->PayloadId.ToString()
				);
				return true;
			}

			if (BuildComponent)
			{
				BuildComponent->SelectBuildingToConstruct(Category);
				RefreshDynamicMappingContexts();
			}
			return true;
		}

	case ETWCommandType::ProduceUnit:
	case ETWCommandType::Research:
		ServerHandleCommandById(CommandId);
		return true;

	default:
		return false;
	}
}

bool ATWPlayerController::TryResolveBuildingCategoryFromPayload(
	FName PayloadId,
	EBuildingCategory& OutCategory
) const
{
	OutCategory = EBuildingCategory::None;

	if (PayloadId == TEXT("Wood") || PayloadId == TEXT("WoodResourceProduction"))
	{
		OutCategory = EBuildingCategory::WoodResourceProduction;
		return true;
	}

	if (PayloadId == TEXT("Stone") || PayloadId == TEXT("StoneResourceProduction"))
	{
		OutCategory = EBuildingCategory::StoneResourceProduction;
		return true;
	}

	if (PayloadId == TEXT("Population") || PayloadId == TEXT("PopulationProduction"))
	{
		OutCategory = EBuildingCategory::PopulationProduction;
		return true;
	}

	if (PayloadId == TEXT("Troop") || PayloadId == TEXT("TroopProduction"))
	{
		OutCategory = EBuildingCategory::TroopProduction;
		return true;
	}

	if (PayloadId == TEXT("Upgrade"))
	{
		OutCategory = EBuildingCategory::Upgrade;
		return true;
	}

	if (PayloadId == TEXT("Blocking"))
	{
		OutCategory = EBuildingCategory::Blocking;
		return true;
	}

	return false;
}

void ATWPlayerController::SetArmedCommandId(FName InCommandId)
{
	ArmedCommandId = InCommandId;
	UpdateInputOverlayState();
}

void ATWPlayerController::ClearArmedCommandId()
{
	ArmedCommandId = NAME_None;
	UpdateInputOverlayState();
}

void ATWPlayerController::HandleCommandById(FName CommandId)
{
	UE_LOG(LogTemp, Log, TEXT("[Command] HandleCommandById: %s"), *CommandId.ToString());

	if (CommandId.IsNone())
	{
		return;
	}

	const FUICommandMetaRow* CommandMeta = FindCommandMetaRowFromTable(CommandId);
	if (!CommandMeta)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Command] CommandMeta not found: %s"), *CommandId.ToString());
		return;
	}
	if (CommandId == TEXT("HeroSkill"))
	{
		ATWHeroUnitBase* OwnedHero = GetOwnedHeroUnit();
		if (!OwnedHero || !IsOwnedHeroCurrentlySelected())
		{
			return;
		}

		if (OwnedHero->GetRemainingCooldownTime() > KINDA_SMALL_NUMBER)
		{
			return;
		}

		OwnedHero->UseSkill();
		RefreshUIBridge();
		return;
	}
	if (!CanExecuteCommand(CommandMeta, CommandId))
	{
		return;
	}

	if (TryHandleImmediateCommand(CommandMeta, CommandId))
	{
		return;
	}

	if (TryHandleArmedCommand(CommandMeta, CommandId))
	{
		return;
	}

	if (TryHandleServerRoutedCommand(CommandMeta, CommandId))
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[Command] Unhandled command: %s / Type=%d / Route=%d"),
		*CommandId.ToString(),
		static_cast<int32>(CommandMeta->CommandType),
		static_cast<int32>(CommandMeta->CommandRoute)
	);
}

void ATWPlayerController::ServerHandleCommandById_Implementation(FName CommandId)
{
	if (CommandId.IsNone())
	{
		return;
	}

	const FUICommandMetaRow* CommandMeta = FindCommandMetaRowFromTable(CommandId);
	if (!CommandMeta)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Command][Server] CommandMeta not found: %s"), *CommandId.ToString());
		return;
	}

	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Command][Server] PlayerState not found: %s"), *CommandId.ToString());
		return;
	}

	switch (CommandMeta->CommandType)
	{
	case ETWCommandType::ProduceUnit:
	{
		ATWBaseBuilding* CurrentSelectedBuilding = GetSelectedBuilding();
		if (!IsValid(CurrentSelectedBuilding))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Command][Server] Produce failed: no selected building"));
			return;
		}

		if (CurrentSelectedBuilding->OwnerPlayerSlot != TWPS->PlayerSlot)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Command][Server] Produce rejected: selected building is not mine | CommandId=%s | BuildingOwner=%d | MySlot=%d"),
				*CommandId.ToString(),
				CurrentSelectedBuilding->OwnerPlayerSlot,
				TWPS->PlayerSlot
			);
			return;
		}

		if (ATWPopulationBuilding* PopulationBuilding = Cast<ATWPopulationBuilding>(CurrentSelectedBuilding))
		{
			const int8 bEnqueueSuccess = PopulationBuilding->RequestEnqueuePopulation();
			if (bEnqueueSuccess != 0)
			{
				RefreshUIBridge();
				ClientForceRefreshSelectionBridge();
			}
			return;
		}

		bool bProduced = false;

		if (TryInvokeBuildingProduceById(CurrentSelectedBuilding, CommandMeta->PayloadId))
		{
			bProduced = true;
		}
		else if (TryInvokeBuildingProduceFallback(CurrentSelectedBuilding))
		{
			bProduced = true;
		}

		if (bProduced)
		{
			RefreshUIBridge();
			ClientForceRefreshSelectionBridge();
		}
		return;
	}
	
	case ETWCommandType::Research:
		{
			ATWBaseBuilding* CurrentSelectedBuilding = GetSelectedBuilding();
			if (!IsValid(CurrentSelectedBuilding))
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command][Server] Research failed: no selected building | CommandId=%s"),
					*CommandId.ToString()
				);
				return;
			}

			if (CurrentSelectedBuilding->OwnerPlayerSlot != TWPS->PlayerSlot)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command][Server] Research rejected: selected building is not mine | CommandId=%s | BuildingOwner=%d | MySlot=%d"),
					*CommandId.ToString(),
					CurrentSelectedBuilding->OwnerPlayerSlot,
					TWPS->PlayerSlot
				);
				return;
			}

			ATWUpgradeBuilding* TargetUpgradeBuilding = Cast<ATWUpgradeBuilding>(CurrentSelectedBuilding);
			if (!TargetUpgradeBuilding)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command][Server] Research failed: selected building is not ATWUpgradeBuilding | CommandId=%s"),
					*CommandId.ToString()
				);
				return;
			}

			if (CommandMeta->PayloadId.IsNone() || CommandMeta->PayloadId == TEXT("None"))
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command][Server] Research failed: invalid payload for %s"),
					*CommandId.ToString()
				);
				return;
			}

			const int8 bStarted = TargetUpgradeBuilding->RequestStartUpgrade(CommandMeta->PayloadId);
			if (bStarted != 0)
			{
				RefreshUIBridge();
				ClientForceRefreshSelectionBridge();
			}
			else
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[Command][Server] Research request rejected | CommandId=%s | Payload=%s"),
					*CommandId.ToString(),
					*CommandMeta->PayloadId.ToString()
				);
			}
			return;
		}

	default:
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Command][Server] Unsupported type: %s / Type=%d"),
			*CommandId.ToString(),
			static_cast<int32>(CommandMeta->CommandType)
		);
		return;
	}
	}
}

void ATWPlayerController::Client_ShowGameResult_Implementation(int32 GameResult)
{
	TSubclassOf<UUserWidget> CurrentWidget = nullptr;
	
	if (GameResult == 1)
	{
		CurrentWidget = VictoryWidgetClass;
	}
	else if (GameResult == 0)
	{
		CurrentWidget = DefeatWidgetClass;
	}
	else
	{
		return;
	}
	
	if (CurrentWidget)
	{
		UUserWidget* ResultUI = CreateWidget<UUserWidget>(this, CurrentWidget);
		if (ResultUI)
		{
			ResultUI->AddToViewport();
		}
	}
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ATWPlayerController::ToggleMenu()
{
	if (!MenuWidgetInstance && MenuWidgetClass)
	{
		MenuWidgetInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	}
	
	if (MenuWidgetInstance)
	{
		if (!MenuWidgetInstance->IsInViewport())
		{
			MenuWidgetInstance->AddToViewport();
			
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(MenuWidgetInstance->GetCachedWidget());
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
		}
		else
		{
			MenuWidgetInstance->RemoveFromParent();
			MenuWidgetInstance = nullptr;
			
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
	}
}

void ATWPlayerController::Client_ShowMenu_Implementation(bool Open)
{
	
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
	HandleCommandById(TEXT("BuildMenu"));
}

void ATWPlayerController::OnSelectWoodBuildingCommandAction()
{
	HandleCommandById(TEXT("BuildWood"));
}

void ATWPlayerController::OnSelectStoneBuildingCommandAction()
{
	HandleCommandById(TEXT("BuildStone"));
}

void ATWPlayerController::OnSelectPopulationBuildingCommandAction()
{
	HandleCommandById(TEXT("BuildPopulation"));
}

void ATWPlayerController::OnSelectTroopBuildingCommandAction()
{
	HandleCommandById(TEXT("BuildTroop"));
}

void ATWPlayerController::OnSelectUpgradeBuildingCommandAction()
{
	HandleCommandById(TEXT("BuildUpgrade"));
}

void ATWPlayerController::OnSelectBlockingBuildingCommandAction()
{
	HandleCommandById(TEXT("BuildBlocking"));
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
		UE_LOG(LogTemp, Warning, TEXT("[TestUpgrade] Failed: PlayerState is null"));
		return;
	}

	const int32 MyPlayerSlot = TWPS->PlayerSlot;
	ATWUpgradeBuilding* TargetUpgradeBuilding = nullptr;

	if (ATWBaseBuilding* CurrentSelectedBuilding = GetSelectedBuilding())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[TestUpgrade] SelectedBuilding=%s | BuildingOwner=%d | MySlot=%d"),
			*GetNameSafe(CurrentSelectedBuilding),
			CurrentSelectedBuilding->OwnerPlayerSlot,
			MyPlayerSlot
		);

		ATWUpgradeBuilding* SelectedUpgradeBuilding = Cast<ATWUpgradeBuilding>(CurrentSelectedBuilding);
		if (!SelectedUpgradeBuilding)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[TestUpgrade] Rejected: selected building is not ATWUpgradeBuilding")
			);
			return;
		}

		if (SelectedUpgradeBuilding->OwnerPlayerSlot != MyPlayerSlot)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[TestUpgrade] Rejected: selected upgrade building is not mine | BuildingOwner=%d | MySlot=%d"),
				SelectedUpgradeBuilding->OwnerPlayerSlot,
				MyPlayerSlot
			);
			return;
		}

		TargetUpgradeBuilding = SelectedUpgradeBuilding;
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[TestUpgrade] No selected building. Searching owned upgrade building... | MySlot=%d"),
			MyPlayerSlot
		);

		for (TActorIterator<ATWUpgradeBuilding> It(GetWorld()); It; ++It)
		{
			ATWUpgradeBuilding* UpgradeBuilding = *It;
			if (!UpgradeBuilding)
			{
				continue;
			}

			UE_LOG(
				LogTemp,
				Verbose,
				TEXT("[TestUpgrade] Candidate=%s | Owner=%d | MySlot=%d"),
				*GetNameSafe(UpgradeBuilding),
				UpgradeBuilding->OwnerPlayerSlot,
				MyPlayerSlot
			);

			if (UpgradeBuilding->OwnerPlayerSlot != MyPlayerSlot)
			{
				continue;
			}

			TargetUpgradeBuilding = UpgradeBuilding;
			break;
		}
	}

	if (!TargetUpgradeBuilding)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[TestUpgrade] Failed: no valid owned upgrade building found | MySlot=%d"),
			MyPlayerSlot
		);
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[TestUpgrade] Execute: Building=%s | Owner=%d | MySlot=%d"),
		*GetNameSafe(TargetUpgradeBuilding),
		TargetUpgradeBuilding->OwnerPlayerSlot,
		MyPlayerSlot
	);

	const int8 bStarted = TargetUpgradeBuilding->RequestStartUpgrade(TEXT("Upgrade_Damage"));
	if (bStarted != 0)
	{
		RefreshUIBridge();
		ClientForceRefreshSelectionBridge();
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[TestUpgrade] RequestStartUpgrade failed | Building=%s"),
			*GetNameSafe(TargetUpgradeBuilding)
		);
	}
}
#pragma endregion

#pragma region Input Overlay
void ATWPlayerController::UpdateInputOverlayState()
{
	if (!PlayerUIControllerComponent)
	{
		return;
	}

	PlayerUIControllerComponent->UpdateInputOverlayState(ArmedCommandId);
}

void ATWPlayerController::UpdateDragSelectionOverlay()
{
	FVector2D ScaledMousePos = FVector2D::ZeroVector;
	const bool bHasScaledMouse =
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

	if (PlayerUIControllerComponent)
	{
		PlayerUIControllerComponent->UpdateDragSelectionOverlay(
			bIsDraggingSelectionVisual,
			DragStartScreenPosition,
			CurrentMouseScreenPosition
		);
	}
}

void ATWPlayerController::UpdateCursorOverlayPosition()
{
	FVector2D ScaledMousePos = FVector2D::ZeroVector;
	bHasValidMousePositionThisFrame =
		UWidgetLayoutLibrary::GetMousePositionScaledByDPI(this, ScaledMousePos.X, ScaledMousePos.Y);

	if (bHasValidMousePositionThisFrame)
	{
		CurrentFrameMouseScreenPosition = ScaledMousePos;
		LastValidMouseScreenPosition = ScaledMousePos;
	}

	FVector2D RawMousePos = FVector2D::ZeroVector;
	bHasValidRawMousePositionThisFrame = GetMousePosition(RawMousePos.X, RawMousePos.Y);

	if (bHasValidRawMousePositionThisFrame)
	{
		CurrentFrameRawMousePosition = RawMousePos;
	}

	if (PlayerUIControllerComponent)
	{
		PlayerUIControllerComponent->UpdateCursorOverlayPosition(
			LastValidMouseScreenPosition,
			bWasEdgeScrollingLastFrame
		);
	}
}

#pragma endregion