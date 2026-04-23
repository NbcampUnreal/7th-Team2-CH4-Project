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
#include "UI/Widgets/TWAlertWidget.h"

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
#include "Core/TWGameMode.h"
#include "HeroUnit/TWHeroUnitBase.h"
#include "Kismet/GameplayStatics.h"
#include "HeroUnit/TWHeroProjectile.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Data/TWHeroTableRowBase.h"
#include "Particles/ParticleSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "FOW/TWFogManager.h"
#include "Log/TWLogCategory.h"

namespace
{
	constexpr float SingleSelectionHitRadius = 100.f;

	static bool AreSelectionSummaryItemsEqual(
		const TArray<FSelectionSummaryItemViewModel>& A,
		const TArray<FSelectionSummaryItemViewModel>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].EntityId != B[Index].EntityId)
			{
				return false;
			}

			if (A[Index].DisplayName != B[Index].DisplayName)
			{
				return false;
			}

			if (A[Index].Count != B[Index].Count)
			{
				return false;
			}

			if (A[Index].CountText != B[Index].CountText)
			{
				return false;
			}
		}

		return true;
	}

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

	static bool TryGetSelectionDataFromEntityHandle(
		UWorld* World,
		const FMassEntityHandle& EntityHandle,
		FName& OutUnitId,
		FMassNetworkID& OutNetId,
		int32* OutOwnerPlayerSlot = nullptr,
		FTWUnitStatus* OutStatus = nullptr)
	{
		OutUnitId = NAME_None;
		OutNetId = FMassNetworkID();

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
		if (!EntityManager.IsEntityValid(EntityHandle) || !EntityManager.IsEntityActive(EntityHandle))
		{
			return false;
		}

		const FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(EntityHandle);
		if (!UnitFragment)
		{
			return false;
		}

		OutUnitId = UnitFragment->GetUnitID();
		if (OutUnitId.IsNone())
		{
			return false;
		}

		if (OutOwnerPlayerSlot)
		{
			*OutOwnerPlayerSlot = UnitFragment->GetOwner();
		}

		if (const FMassNetworkIDFragment* NetworkIdFragment =
			EntityManager.GetFragmentDataPtr<FMassNetworkIDFragment>(EntityHandle))
		{
			OutNetId = NetworkIdFragment->NetID;
		}

		if (OutStatus)
		{
			if (const FTWStatusFragment* StatusFragment = EntityManager.GetFragmentDataPtr<FTWStatusFragment>(EntityHandle))
			{
				*OutStatus = StatusFragment->GetStatus();
			}
		}

		return true;
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
	
	static bool IsHeroUnitId(const FName UnitId)
	{
		return
			UnitId == TEXT("DragonKnight") ||
			UnitId == TEXT("Markman") ||
			UnitId == TEXT("Astrologian");
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

	if (IsLocalController())
	{
		EnableCheats();
	}
	
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

	// 추가: BeginPlay 시점에 이미 Pawn이 있으면 pending 포커스 적용
	if (bHasPendingStartFocusLocation)
	{
		ClientFocusStartLocation_Implementation(PendingStartFocusLocation);
	}
}

void ATWPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!bHasPendingStartFocusLocation || !IsValid(InPawn))
	{
		return;
	}

	FVector NewLocation = PendingStartFocusLocation;
	NewLocation.Z = InPawn->GetActorLocation().Z;

	InPawn->SetActorLocation(NewLocation);

	bHasPendingStartFocusLocation = false;
	PendingStartFocusLocation = FVector::ZeroVector;
}

void ATWPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateCursorOverlayPosition();
	UpdateHeroSkillTargeting();

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

	RefreshLocalSelectionRuntimeData();
	HandleHiddenEnemySelectionByFog();

	if (PlayerSelectionVisualComponent)
	{
		PlayerSelectionVisualComponent->TickVisuals(DeltaSeconds);
	}
}
void ATWPlayerController::ClientMoveCameraToStartLocation_Implementation(
	const FVector& InTargetLocation,
	const FRotator& InTargetRotation
)
{
	APawn* ControlledPawn = GetPawn();
	if (IsValid(ControlledPawn))
	{
		ControlledPawn->SetActorLocation(InTargetLocation);
		ControlledPawn->SetActorRotation(InTargetRotation);
	}

	SetControlRotation(InTargetRotation);

	SetInitialLocationAndRotation(InTargetLocation, InTargetRotation);
}

void ATWPlayerController::ClientFocusStartLocation_Implementation(const FVector& InWorldLocation)
{
	APawn* MyPawn = GetPawn();
	if (!IsValid(MyPawn))
	{
		bHasPendingStartFocusLocation = true;
		PendingStartFocusLocation = InWorldLocation;

		UE_LOG(
			LogTWCommand,
			Warning,
			TEXT("[PlayerController] FocusStart deferred: Pawn not ready | Target=%s"),
			*InWorldLocation.ToString()
		);
		return;
	}

	FVector NewLocation = InWorldLocation;
	NewLocation.Z = MyPawn->GetActorLocation().Z;

	MyPawn->SetActorLocation(NewLocation);

	bHasPendingStartFocusLocation = false;
	PendingStartFocusLocation = FVector::ZeroVector;
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
	check(IsValid(IA_Menu));
	
	check(IsValid(MoveCommandAction));
	check(IsValid(AttackCommandAction));
	check(IsValid(HoldCommandAction));
	check(IsValid(HeroSkillCommandAction));
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
	check(IsValid(QueueHotkeySAction));
	check(IsValid(QueueHotkeyDAction));
	check(IsValid(QueueHotkeyZAction));
	check(IsValid(QueueHotkeyXAction));
	check(IsValid(QueueHotkeyCAction));

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Started, this, &ThisClass::OnStartLeftMouseAction);
	EnhancedInputComponent->BindAction(LeftMouseAction, ETriggerEvent::Completed, this, &ThisClass::OnEndLeftMouseAction);
	EnhancedInputComponent->BindAction(RightMouseAction, ETriggerEvent::Started, this, &ThisClass::OnRightMouseAction);
	EnhancedInputComponent->BindAction(IA_Menu, ETriggerEvent::Started, this, &ATWPlayerController::ToggleMenu);
	EnhancedInputComponent->BindAction(ToggleBuildModeAction, ETriggerEvent::Started, this, &ThisClass::OnToggleBuildModeAction);

	EnhancedInputComponent->BindAction(MoveCommandAction, ETriggerEvent::Started, this, &ThisClass::OnMoveCommandAction);
	EnhancedInputComponent->BindAction(AttackCommandAction, ETriggerEvent::Started, this, &ThisClass::OnAttackCommandAction);
	EnhancedInputComponent->BindAction(HoldCommandAction, ETriggerEvent::Started, this, &ThisClass::OnHoldCommandAction);
	EnhancedInputComponent->BindAction(HeroSkillCommandAction, ETriggerEvent::Started, this, &ThisClass::OnHeroSkillCommandAction);
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
	EnhancedInputComponent->BindAction(QueueHotkeySAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyS);
	EnhancedInputComponent->BindAction(QueueHotkeyDAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyD);
	EnhancedInputComponent->BindAction(QueueHotkeyZAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyZ);
	EnhancedInputComponent->BindAction(QueueHotkeyXAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyX);
	EnhancedInputComponent->BindAction(QueueHotkeyCAction, ETriggerEvent::Started, this, &ThisClass::OnQueueHotkeyC);
}

#pragma region 마우스
void ATWPlayerController::OnStartLeftMouseAction(const FInputActionValue& InputActionValue)
{
	if (bHeroSkillTargetingMode)
	{
		ConfirmHeroSkillTargeting();
		bConsumeLeftMouseRelease = true;
		return;
	}
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
			if (IsEnemyBuildingHiddenByFog(ClickedBuilding))
			{
				ServerHandleBuildingSelect(nullptr);
				return;
			}

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
	if (bHeroSkillTargetingMode)
	{
		CancelHeroSkillTargeting();
		return;
	}
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

void ATWPlayerController::OnHeroSkillCommandAction(const FInputActionValue& InputActionValue)
{
	if (!ShouldUseUnitCommandContext())
	{
		return;
	}

	if (bBuildShortcutModeActive)
	{
		return;
	}

	if (BuildComponent && BuildComponent->GetBuildMode())
	{
		return;
	}

	HandleCommandById(TEXT("HeroSkill"));
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

			if (IsLocalController())
			{
				ClientApplyUnitSelection_Implementation(
					NetworkIds,
					PrimaryHealth,
					bHasPrimaryHealth,
					InSelectedOwnerPlayerSlot);
			}
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
			
			if (OtherPS->PlayerSlot != TWPS->PlayerSlot && IsLocationHiddenByFog(EntityLocation))
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

	if (IsLocalController())
	{
		ClientClearSelection_Implementation();
	}
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
	
	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

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

			if (IsLocalController())
			{
				ClientApplyUnitSelection_Implementation(
					NetworkIds,
					PrimaryHealth,
					bHasPrimaryHealth,
					InSelectedOwnerPlayerSlot);
			}
		};

	auto FindBestBuildingInRect =
		[this, World, &RectMin, &RectMax, &RectCenter](const bool bRequireOwnedByMe, const int32 InMyPlayerSlot)
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
				
				if (!bRequireOwnedByMe && IsEnemyBuildingHiddenByFog(Building))
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
	TArray<FMassEntityHandle> OwnedHeroEntities;
	TArray<FMassEntityHandle> OwnedNormalEntities;

	if (UnitSubsystem->GetEntitiesInRectangle(StartLocation, EndLocation, OwnedEntities, TWPS->PlayerSlot))
	{
		for (const FMassEntityHandle& Entity : OwnedEntities)
		{
			if (!EntityManager.IsEntityValid(Entity) || !EntityManager.IsEntityActive(Entity))
			{
				continue;
			}

			const FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
			if (!UnitFragment)
			{
				continue;
			}

			if (IsHeroUnitId(UnitFragment->GetUnitID()))
			{
				OwnedHeroEntities.Add(Entity);
			}
			else
			{
				OwnedNormalEntities.Add(Entity);
			}
		}
	}

	if (OwnedHeroEntities.Num() > 0)
	{
		ApplyUnitSelection(OwnedHeroEntities, TWPS->PlayerSlot);
		return;
	}

	if (OwnedNormalEntities.Num() > 0)
	{
		ApplyUnitSelection(OwnedNormalEntities, TWPS->PlayerSlot);
		return;
	}

	if (ATWBaseBuilding* OwnedBuilding = FindBestBuildingInRect(true, TWPS->PlayerSlot))
	{
		ServerHandleBuildingSelect(OwnedBuilding);
		return;
	}

	FMassEntityHandle BestEnemyHeroEntity;
	float BestEnemyHeroDistanceSq = TNumericLimits<float>::Max();
	int32 BestEnemyHeroOwnerSlot = INDEX_NONE;

	FMassEntityHandle BestEnemyNormalEntity;
	float BestEnemyNormalDistanceSq = TNumericLimits<float>::Max();
	int32 BestEnemyNormalOwnerSlot = INDEX_NONE;

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
			if (!EntityManager.IsEntityValid(EnemyEntity) || !EntityManager.IsEntityActive(EnemyEntity))
			{
				continue;
			}

			const FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(EnemyEntity);
			if (!UnitFragment)
			{
				continue;
			}

			FVector EnemyLocation = FVector::ZeroVector;
			if (!TryGetEntityWorldLocation(World, EnemyEntity, EnemyLocation))
			{
				continue;
			}

			if (!IsPointInsideSelectionRect2D(EnemyLocation, RectMin, RectMax))
			{
				continue;
			}
			
			if (IsLocationHiddenByFog(EnemyLocation))
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared2D(EnemyLocation, RectCenter);

			if (IsHeroUnitId(UnitFragment->GetUnitID()))
			{
				if (DistanceSq < BestEnemyHeroDistanceSq)
				{
					BestEnemyHeroDistanceSq = DistanceSq;
					BestEnemyHeroEntity = EnemyEntity;
					BestEnemyHeroOwnerSlot = OtherPS->PlayerSlot;
				}
			}
			else
			{
				if (DistanceSq < BestEnemyNormalDistanceSq)
				{
					BestEnemyNormalDistanceSq = DistanceSq;
					BestEnemyNormalEntity = EnemyEntity;
					BestEnemyNormalOwnerSlot = OtherPS->PlayerSlot;
				}
			}
		}
	}

	if (BestEnemyHeroEntity.IsSet())
	{
		ApplyUnitSelection({ BestEnemyHeroEntity }, BestEnemyHeroOwnerSlot);
		return;
	}

	if (BestEnemyNormalEntity.IsSet())
	{
		ApplyUnitSelection({ BestEnemyNormalEntity }, BestEnemyNormalOwnerSlot);
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

	if (IsLocalController())
	{
		ClientClearSelection_Implementation();
	}
}

void ATWPlayerController::ServerHandleBuildingSelect_Implementation(ATWBaseBuilding* TargetBuilding)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));

	ServerSelectedEntities.Empty();
	SelectedBuilding = TargetBuilding;

	if (!IsValid(TargetBuilding))
	{
		ClientClearSelection();

		if (IsLocalController())
		{
			ClientClearSelection_Implementation();
		}
		return;
	}

	ClientApplyBuildingSelection(TargetBuilding);

	if (IsLocalController())
	{
		ClientApplyBuildingSelection_Implementation(TargetBuilding);
	}
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

bool ATWPlayerController::IsLocationHiddenByFog(const FVector& InWorldLocation) const
{
	if (!IsLocalController())
	{
		return false;
	}

	ATWFogManager* FogManager = nullptr;
	for (TActorIterator<ATWFogManager> It(GetWorld()); It; ++It)
	{
		FogManager = *It;
		break;
	}

	if (!FogManager)
	{
		return false;
	}

	return !FogManager->IsLocationVisible(InWorldLocation);
}

bool ATWPlayerController::IsEnemyBuildingHiddenByFog(const ATWBaseBuilding* InBuilding) const
{
	if (!IsLocalController())
	{
		return false;
	}

	if (!IsValid(InBuilding))
	{
		return false;
	}

	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return false;
	}

	if (InBuilding->OwnerPlayerSlot == LocalPS->PlayerSlot)
	{
		return false;
	}

	return IsLocationHiddenByFog(InBuilding->GetActorLocation());
}

bool ATWPlayerController::ShouldClearCurrentSelectionByFog() const
{
	if (!IsLocalController())
	{
		return false;
	}

	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return false;
	}

	if (SelectedBuilding)
	{
		if (SelectedBuilding->OwnerPlayerSlot == LocalPS->PlayerSlot)
		{
			return false;
		}

		return IsLocationHiddenByFog(SelectedBuilding->GetActorLocation());
	}

	if (ClientSelectedEntities.Num() != 1)
	{
		return false;
	}

	if (LocalSelectedOwnerPlayerSlot == LocalPS->PlayerSlot)
	{
		return false;
	}

	FVector SelectedUnitLocation = FVector::ZeroVector;
	bool bResolvedLocation = false;

	if (HasAuthority() && ServerSelectedEntities.Num() == 1 && ServerSelectedEntities[0].IsSet())
	{
		bResolvedLocation = TryGetEntityWorldLocation(
			GetWorld(),
			ServerSelectedEntities[0],
			SelectedUnitLocation
		);
	}
	else
	{
		const UTWUnitSubsystem* UnitSubsystem =
			GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;
		if (!UnitSubsystem)
		{
			return false;
		}

		const FMassNetworkID SelectedNetId = ClientSelectedEntities[0];
		if (!SelectedNetId.IsValid())
		{
			return false;
		}

		bResolvedLocation = UnitSubsystem->TryGetUnitVisualLocation(
			SelectedNetId,
			SelectedUnitLocation
		);

		if (!bResolvedLocation)
		{
			bResolvedLocation = UnitSubsystem->TryGetUnitHPBarWorldLocation(
				SelectedNetId,
				SelectedUnitLocation
			);
		}
	}

	if (!bResolvedLocation)
	{
		return false;
	}

	return IsLocationHiddenByFog(SelectedUnitLocation);
}

void ATWPlayerController::HandleHiddenEnemySelectionByFog()
{
	if (!ShouldClearCurrentSelectionByFog())
	{
		return;
	}

	ServerHandleBuildingSelect(nullptr);
}

bool ATWPlayerController::ShouldRejectIncomingUnitSelectionByFog(const TArray<FMassNetworkID>& InNetworkIds,
	int32 InSelectedOwnerPlayerSlot) const
{
	if (!IsLocalController())
	{
		return false;
	}

	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return false;
	}

	if (InSelectedOwnerPlayerSlot == LocalPS->PlayerSlot)
	{
		return false;
	}

	if (InNetworkIds.Num() != 1)
	{
		return false;
	}

	const UTWUnitSubsystem* UnitSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	if (!UnitSubsystem)
	{
		return false;
	}

	const FMassNetworkID SelectedNetId = InNetworkIds[0];
	if (!SelectedNetId.IsValid())
	{
		return false;
	}

	FVector SelectedUnitLocation = FVector::ZeroVector;

	bool bResolvedLocation = UnitSubsystem->TryGetUnitVisualLocation(
		SelectedNetId,
		SelectedUnitLocation
	);

	if (!bResolvedLocation)
	{
		bResolvedLocation = UnitSubsystem->TryGetUnitHPBarWorldLocation(
			SelectedNetId,
			SelectedUnitLocation
		);
	}

	if (!bResolvedLocation)
	{
		return false;
	}

	return IsLocationHiddenByFog(SelectedUnitLocation);
}

bool ATWPlayerController::ShouldRejectIncomingBuildingSelectionByFog(const ATWBaseBuilding* InBuilding) const
{
	if (!IsLocalController())
	{
		return false;
	}

	if (!IsValid(InBuilding))
	{
		return false;
	}

	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return false;
	}

	if (InBuilding->OwnerPlayerSlot == LocalPS->PlayerSlot)
	{
		return false;
	}

	return IsLocationHiddenByFog(InBuilding->GetActorLocation());
}

void ATWPlayerController::HandleUnitKilledSelectionClear(const FMassEntityHandle& DeadEntity)
{
	if (!HasAuthority() || !DeadEntity.IsSet())
	{
		return;
	}

	bool bWasSelected = false;

	for (const FMassEntityHandle& SelectedEntity : ServerSelectedEntities)
	{
		if (SelectedEntity == DeadEntity)
		{
			bWasSelected = true;
			break;
		}
	}

	if (!bWasSelected)
	{
		return;
	}

	ServerSelectedEntities.Empty();
	SelectedBuilding = nullptr;

	ClientClearSelectionByDeath();

	if (IsLocalController())
	{
		ClientClearSelectionByDeath_Implementation();
	}
}

void ATWPlayerController::ClientClearSelectionByDeath_Implementation()
{
	ClientClearSelection_Implementation();
}
#pragma endregion

#pragma region 병력 스폰 대기열 / 인구 수 대기열 / 연구소 대기열
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

void ATWPlayerController::OnQueueHotkeyS(const FInputActionValue& Value)
{
	HandleBuildingProductionSlot(4);
}

void ATWPlayerController::OnQueueHotkeyD(const FInputActionValue& Value)
{
	HandleBuildingProductionSlot(5);
}

void ATWPlayerController::OnQueueHotkeyZ(const FInputActionValue& Value)
{
	HandleBuildingProductionSlot(6);
}

void ATWPlayerController::OnQueueHotkeyX(const FInputActionValue& Value)
{
	HandleBuildingProductionSlot(7);
}

void ATWPlayerController::OnQueueHotkeyC(const FInputActionValue& Value)
{
	HandleBuildingProductionSlot(8);
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

FVector ATWPlayerController::GetMouseWorldLocation() const
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	return Hit.bBlockingHit ? Hit.ImpactPoint : FVector::ZeroVector;
}
bool ATWPlayerController::TryGetSelectedHeroUnitId(FName& OutHeroUnitId) const
{
	OutHeroUnitId = NAME_None;

	if (!IsLocalController() || SelectedBuilding != nullptr)
	{
		return false;
	}

	const ATWPlayerState* PS = GetPlayerState<ATWPlayerState>();
	if (!PS)
	{
		return false;
	}

	const FName PlayerStateHeroUnitId = PS->GetSelectedHeroUnitId();
	if (PlayerStateHeroUnitId.IsNone())
	{
		return false;
	}

	if (LocalSelectedOwnerPlayerSlot != PS->PlayerSlot)
	{
		return false;
	}

	if (ClientSelectedEntities.Num() != 1)
	{
		return false;
	}

	FName SelectedUnitId = LocalPrimarySelectedUnitId;
	FMassNetworkID PrimaryNetId = LocalPrimarySelectedUnitNetId;
	if (SelectedUnitId.IsNone() || !PrimaryNetId.IsValid())
	{
		if (!TryResolveLocalPrimarySelectedUnitFromClientSelection(SelectedUnitId, PrimaryNetId))
		{
			return false;
		}
	}

	if (SelectedUnitId == PlayerStateHeroUnitId)
	{
		OutHeroUnitId = SelectedUnitId;
		return true;
	}

	const UTWUnitSubsystem* UnitSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	ATWHeroUnitBase* OwnedHero = GetOwnedHeroUnit();
	if (!UnitSubsystem || !OwnedHero || !PrimaryNetId.IsValid())
	{
		return false;
	}

	FVector SelectedLocation = FVector::ZeroVector;
	if (!UnitSubsystem->TryGetUnitVisualLocation(PrimaryNetId, SelectedLocation))
	{
		if (!UnitSubsystem->TryGetUnitHPBarWorldLocation(PrimaryNetId, SelectedLocation))
		{
			return false;
		}
	}

	const float DistSq = FVector::DistSquared2D(SelectedLocation, OwnedHero->GetActorLocation());
	if (DistSq <= FMath::Square(200.0f))
	{
		OutHeroUnitId = PlayerStateHeroUnitId;
		return true;
	}

	return false;
}


bool ATWPlayerController::TryFindOwnedHeroEntityByUnitId(FName InHeroUnitId, FMassEntityHandle& OutEntityHandle) const
{
	OutEntityHandle = FMassEntityHandle();

	if (!HasAuthority() || InHeroUnitId.IsNone())
	{
		return false;
	}

	const ATWPlayerState* PS = GetPlayerState<ATWPlayerState>();
	if (!PS)
	{
		return false;
	}

	UWorld* World = GetWorld();
	UTWUnitSubsystem* UnitSubsystem = World ? World->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	UMassEntitySubsystem* EntitySubsystem = World ? World->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	if (!World || !UnitSubsystem || !EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	// 1) 서버 선택 목록 안에 이미 엔티티가 있으면 그걸 우선 사용
	for (const FMassEntityHandle& Entity : ServerSelectedEntities)
	{
		if (!EntityManager.IsEntityActive(Entity))
		{
			continue;
		}

		OutEntityHandle = Entity;
		return true;
	}

	// 2) 소유 중인 영웅 액터를 기준으로 근처 엔티티를 찾음
	const ATWHeroUnitBase* OwnedHero = GetOwnedHeroUnit();
	if (!IsValid(OwnedHero))
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[HeroSkill][Server] OwnedHero actor not found"));
		return false;
	}

	const FVector HeroLocation = OwnedHero->GetActorLocation();
	const FVector SearchExtent(200.f, 200.f, 300.f);
	const FVector AreaMin = HeroLocation - SearchExtent;
	const FVector AreaMax = HeroLocation + SearchExtent;

	TArray<FMassEntityHandle> NearbyEntities;
	if (!UnitSubsystem->GetEntitiesInRectangle(AreaMin, AreaMax, NearbyEntities, PS->PlayerSlot))
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[HeroSkill][Server] No nearby entities around hero actor"));
		return false;
	}

	FMassEntityHandle BestEntity;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (const FMassEntityHandle& Entity : NearbyEntities)
	{
		if (!EntityManager.IsEntityActive(Entity))
		{
			continue;
		}

		FVector EntityLocation = FVector::ZeroVector;
		if (!TryGetEntityWorldLocation(World, Entity, EntityLocation))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(EntityLocation, HeroLocation);
		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestEntity = Entity;
		}
	}

	if (!BestEntity.IsSet())
	{
		UE_LOG(
			LogTWCommand,
			Warning,
			TEXT("[HeroSkill][Server] HeroEntity not found | Slot=%d | HeroId=%s"),
			PS->PlayerSlot,
			*InHeroUnitId.ToString()
		);
		return false;
	}

	OutEntityHandle = BestEntity;
	return true;
}

bool ATWPlayerController::TryApplyDamageToBuilding(ATWBaseBuilding* InTargetBuilding, float InDamage) const
{
	if (!IsValid(InTargetBuilding))
	{
		return false;
	}

	InTargetBuilding->ApplyDamageToBuilding(FMath::Max(1, FMath::RoundToInt(InDamage)));
	return true;
}

void ATWPlayerController::NotifyAllPlayersUnitDamaged(const FMassEntityHandle& InTargetEntity, float InVisibleTime) const
{
	if (!GetWorld())
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	if (!EntityManager.IsEntityActive(InTargetEntity))
	{
		return;
	}

	const FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(InTargetEntity);
	const FMassNetworkIDFragment* NetworkIDFragment = EntityManager.GetFragmentDataPtr<FMassNetworkIDFragment>(InTargetEntity);
	if (!UnitFragment || !NetworkIDFragment)
	{
		return;
	}

	const int32 OwnerPlayerSlot = UnitFragment->GetOwner();
	const FMassNetworkID UnitNetId = NetworkIDFragment->NetID;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ATWPlayerController* TargetPC = Cast<ATWPlayerController>(It->Get());
		if (!TargetPC)
		{
			continue;
		}

		TargetPC->ClientNotifyRecentCombatUnitDamaged(UnitNetId, OwnerPlayerSlot, InVisibleTime);
	}
}

void ATWPlayerController::NotifyAllPlayersBuildingDamaged(ATWBaseBuilding* InTargetBuilding, float InVisibleTime) const
{
	if (!GetWorld() || !IsValid(InTargetBuilding))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ATWPlayerController* TargetPC = Cast<ATWPlayerController>(It->Get());
		if (!TargetPC)
		{
			continue;
		}

		TargetPC->ClientNotifyRecentCombatBuildingDamaged(InTargetBuilding, InVisibleTime);
	}
}
FName ATWPlayerController::ResolveHeroSkillRowName(FName InHeroUnitId) const
{
	if (InHeroUnitId.IsNone())
	{
		return NAME_None;
	}

	const UWorld* World = GetWorld();
	const UTWUnitSubsystem* UnitSubsystem = World ? World->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	if (UnitSubsystem)
	{
		if (const FTWUnitTableRowBase* UnitRow = UnitSubsystem->GetUnitTableRowBase(InHeroUnitId))
		{
			if (!UnitRow->HeroSkillRowId.IsNone())
			{
				return UnitRow->HeroSkillRowId;
			}
		}
	}

	return InHeroUnitId;
}

const FTWHeroTableRowBase* ATWPlayerController::FindHeroSkillRow(FName InHeroUnitId) const
{
	if (!HeroSkillDataTable || InHeroUnitId.IsNone())
	{
		return nullptr;
	}

	const FName RowName = ResolveHeroSkillRowName(InHeroUnitId);
	if (RowName.IsNone())
	{
		return nullptr;
	}

	static const FString ContextString(TEXT("ATWPlayerController::FindHeroSkillRow"));
	return HeroSkillDataTable->FindRow<FTWHeroTableRowBase>(RowName, ContextString, false);
}

void ATWPlayerController::EnsureHeroSkillTargetDecal()
{
	if (HeroSkillTargetDecal)
	{
		return;
	}

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	HeroSkillTargetDecal = NewObject<UDecalComponent>(MyPawn, TEXT("HeroSkillTargetDecal"));
	if (!HeroSkillTargetDecal)
	{
		return;
	}

	HeroSkillTargetDecal->SetupAttachment(MyPawn->GetRootComponent());
	HeroSkillTargetDecal->RegisterComponent();
	HeroSkillTargetDecal->SetVisibility(false);
	HeroSkillTargetDecal->SetHiddenInGame(true);
	HeroSkillTargetDecal->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));

	if (HeroSkillDefaultTargetDecalMaterial)
	{
		HeroSkillTargetDecal->SetDecalMaterial(HeroSkillDefaultTargetDecalMaterial);
	}
}

void ATWPlayerController::ShowHeroSkillTargetDecal(float InRadius)
{
	EnsureHeroSkillTargetDecal();

	if (!HeroSkillTargetDecal)
	{
		return;
	}

	HeroSkillTargetDecalRadius = InRadius;
	HeroSkillTargetDecal->DecalSize = FVector(FMath::Max(16.f, InRadius), FMath::Max(16.f, InRadius), FMath::Max(16.f, InRadius));
	HeroSkillTargetDecal->SetVisibility(true);
	HeroSkillTargetDecal->SetHiddenInGame(false);
}

void ATWPlayerController::HideHeroSkillTargetDecal()
{
	if (!HeroSkillTargetDecal)
	{
		return;
	}

	HeroSkillTargetDecal->SetVisibility(false);
	HeroSkillTargetDecal->SetHiddenInGame(true);
}

void ATWPlayerController::UpdateHeroSkillTargetDecal(const FVector& InWorldLocation)
{
	if (!HeroSkillTargetDecal || !bHeroSkillTargetingMode)
	{
		return;
	}

	const FVector DecalLocation = InWorldLocation + FVector(0.f, 0.f, 10.f);
	HeroSkillTargetDecal->SetWorldLocation(DecalLocation);
	HeroSkillTargetDecal->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));
}

float ATWPlayerController::ResolveHeroSkillTargetRadius(FName InHeroUnitId) const
{
	const FTWHeroTableRowBase* HeroSkillRow = FindHeroSkillRow(InHeroUnitId);
	if (!HeroSkillRow)
	{
		return HeroSkillDefaultTargetDecalRadius;
	}

	if (HeroSkillRow->TargetDecalRadius > 0.f)
	{
		return HeroSkillRow->TargetDecalRadius;
	}

	if (HeroSkillRow->DamageRadius > 0.f)
	{
		return HeroSkillRow->DamageRadius;
	}

	if (HeroSkillRow->SkillRange > 0.f)
	{
		return HeroSkillRow->SkillRange;
	}

	return HeroSkillDefaultTargetDecalRadius;
}

float ATWPlayerController::ResolveHeroSkillDamageRadius(const FTWHeroTableRowBase* InHeroSkillRow) const
{
	if (!InHeroSkillRow)
	{
		return HeroSkillDefaultTargetDecalRadius;
	}

	if (InHeroSkillRow->DamageRadius > 0.f)
	{
		return InHeroSkillRow->DamageRadius;
	}

	if (InHeroSkillRow->TargetDecalRadius > 0.f)
	{
		return InHeroSkillRow->TargetDecalRadius;
	}

	if (InHeroSkillRow->SkillRange > 0.f)
	{
		return InHeroSkillRow->SkillRange;
	}

	return HeroSkillDefaultTargetDecalRadius;
}

float ATWPlayerController::ResolveHeroSkillAcquireRadius(const FTWHeroTableRowBase* InHeroSkillRow) const
{
	if (!InHeroSkillRow)
	{
		return HeroSkillDefaultTargetDecalRadius;
	}

	if (InHeroSkillRow->TargetAcquireRadius > 0.f)
	{
		return InHeroSkillRow->TargetAcquireRadius;
	}

	if (InHeroSkillRow->TargetDecalRadius > 0.f)
	{
		return InHeroSkillRow->TargetDecalRadius;
	}

	if (InHeroSkillRow->SkillRange > 0.f)
	{
		return InHeroSkillRow->SkillRange;
	}

	return HeroSkillDefaultTargetDecalRadius;
}

float ATWPlayerController::ResolveHeroSkillImpactRadius(const FTWHeroTableRowBase* InHeroSkillRow) const
{
	if (!InHeroSkillRow)
	{
		return HeroSkillDefaultTargetDecalRadius;
	}

	if (InHeroSkillRow->ImpactDecalRadius > 0.f)
	{
		return InHeroSkillRow->ImpactDecalRadius;
	}

	return ResolveHeroSkillDamageRadius(InHeroSkillRow);
}

float ATWPlayerController::ResolveHeroSkillImpactLifeTime(const FTWHeroTableRowBase* InHeroSkillRow) const
{
	if (InHeroSkillRow && InHeroSkillRow->ImpactDecalLifeTime > 0.f)
	{
		return InHeroSkillRow->ImpactDecalLifeTime;
	}

	return HeroSkillDefaultImpactDecalLifeTime;
}

UMaterialInterface* ATWPlayerController::ResolveHeroSkillTargetDecalMaterial(FName InHeroUnitId) const
{
	const FTWHeroTableRowBase* HeroSkillRow = FindHeroSkillRow(InHeroUnitId);
	if (HeroSkillRow && HeroSkillRow->TargetDecalMaterial)
	{
		return HeroSkillRow->TargetDecalMaterial;
	}

	return HeroSkillDefaultTargetDecalMaterial;
}

UMaterialInterface* ATWPlayerController::ResolveHeroSkillImpactDecalMaterial(FName InHeroUnitId) const
{
	const FTWHeroTableRowBase* HeroSkillRow = FindHeroSkillRow(InHeroUnitId);
	if (HeroSkillRow && HeroSkillRow->ImpactDecalMaterial)
	{
		return HeroSkillRow->ImpactDecalMaterial;
	}

	return HeroSkillDefaultImpactDecalMaterial ? HeroSkillDefaultImpactDecalMaterial : HeroSkillDefaultTargetDecalMaterial;
}

void ATWPlayerController::ApplyHeroSkillTargetDecalStyle(FName InHeroUnitId, float InRadius)
{
	EnsureHeroSkillTargetDecal();

	if (!HeroSkillTargetDecal)
	{
		return;
	}

	if (UMaterialInterface* DecalMaterial = ResolveHeroSkillTargetDecalMaterial(InHeroUnitId))
	{
		HeroSkillTargetDecal->SetDecalMaterial(DecalMaterial);
	}

	ShowHeroSkillTargetDecal(InRadius);
}

void ATWPlayerController::ClientPlayHeroSkillFX_Implementation(
	FName InHeroUnitId,
	FVector InStartLocation,
	FVector InTargetLocation,
	float InRadius)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FTWHeroTableRowBase* HeroSkillRow = FindHeroSkillRow(InHeroUnitId);
	const float EffectRadius = InRadius > 0.f ? InRadius : ResolveHeroSkillImpactRadius(HeroSkillRow);

	if (HeroSkillRow && HeroSkillRow->ImpactNiagaraFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			HeroSkillRow->ImpactNiagaraFX,
			InTargetLocation,
			FRotator::ZeroRotator
		);
	}

	if (HeroSkillRow && HeroSkillRow->ImpactParticleFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			World,
			HeroSkillRow->ImpactParticleFX,
			FTransform(FRotator::ZeroRotator, InTargetLocation)
		);
	}

	if (InHeroUnitId == TEXT("Markman"))
	{
		if (!HeroSkillRow || !HeroSkillRow->ProjectileClass)
		{
			DrawDebugLine(World, InStartLocation, InTargetLocation, FColor::Yellow, false, 0.5f, 0, 2.5f);
			return;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = World->SpawnActor<AActor>(
			HeroSkillRow->ProjectileClass,
			InStartLocation,
			(InTargetLocation - InStartLocation).Rotation(),
			SpawnParams
		);

		if (ATWHeroProjectile* HeroProjectile = Cast<ATWHeroProjectile>(SpawnedActor))
		{
			HeroProjectile->InitializeVisualProjectile(InTargetLocation);
		}
		else if (!SpawnedActor)
		{
			DrawDebugLine(World, InStartLocation, InTargetLocation, FColor::Yellow, false, 0.5f, 0, 2.5f);
		}

		return;
	}

	if (UMaterialInterface* ImpactMaterial = ResolveHeroSkillImpactDecalMaterial(InHeroUnitId))
	{
		UGameplayStatics::SpawnDecalAtLocation(
			World,
			ImpactMaterial,
			FVector(32.f, EffectRadius, EffectRadius),
			InTargetLocation + FVector(0.f, 0.f, 10.f),
			FRotator(-90.f, 0.f, 0.f),
			ResolveHeroSkillImpactLifeTime(HeroSkillRow)
		);
	}

	const bool bShouldDrawDebug = (HeroSkillRow && HeroSkillRow->bUseSkillDebugDraw) || bDrawHeroSkillDebug;
	if (bShouldDrawDebug)
	{
		DrawDebugSphere(
			World,
			InTargetLocation + FVector(0.f, 0.f, 25.f),
			EffectRadius,
			48,
			FColor::Cyan,
			false,
			1.25f,
			0,
			2.0f
		);

		DrawDebugCircle(
			World,
			InTargetLocation + FVector(0.f, 0.f, 10.f),
			EffectRadius,
			64,
			FColor::Green,
			false,
			1.25f,
			0,
			2.0f,
			FVector(1.f, 0.f, 0.f),
			FVector(0.f, 1.f, 0.f),
			false
		);
	}
}

void ATWPlayerController::NotifyAllPlayersHeroSkillFX(
	FName InHeroUnitId,
	const FVector& InStartLocation,
	const FVector& InTargetLocation,
	float InRadius) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ATWPlayerController* TargetPC = Cast<ATWPlayerController>(It->Get());
		if (!TargetPC)
		{
			continue;
		}

		TargetPC->ClientPlayHeroSkillFX(InHeroUnitId, InStartLocation, InTargetLocation, InRadius);
	}
}

void ATWPlayerController::ServerUseHeroSkill_Implementation(FVector InTargetLocation, FName InHeroUnitId)
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}

	if (GetWorld()->GetTimeSeconds() < ServerHeroSkillCooldownEndTime)
	{
		return;
	}

	const FName HeroUnitId = InHeroUnitId;
	if (HeroUnitId.IsNone())
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[HeroSkill][Server] HeroUnitId is None"));
		return;
	}

	UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
	ATWPlayerState* PS = GetPlayerState<ATWPlayerState>();
	if (!UnitSubsystem || !PS)
	{
		return;
	}

	FMassEntityHandle HeroEntity;
	if (!TryFindOwnedHeroEntityByUnitId(HeroUnitId, HeroEntity))
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[HeroSkill][Server] HeroEntity not found for UnitId=%s"), *HeroUnitId.ToString());
		return;
	}

	const FTWHeroTableRowBase* HeroRow = FindHeroSkillRow(HeroUnitId);
	if (!HeroRow)
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[HeroSkill][Server] HeroRow not found: %s"), *HeroUnitId.ToString());
		return;
	}

	FVector HeroLocation = FVector::ZeroVector;
	if (!UnitSubsystem->TryGetUnitWorldLocation(HeroEntity, HeroLocation))
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[HeroSkill][Server] hero location resolve failed"));
		return;
	}

	const FTWUnitStatus HeroRuntimeStatus = UnitSubsystem->GetUnitCurrentStatus(HeroEntity, PS->PlayerSlot);
	const float BaseDamage = HeroRuntimeStatus.GetStatus(ETWStatusType::Damage);
	const float SkillDamage = FMath::Max(1.f, BaseDamage * HeroRow->SkillMultiplier);

	if (HeroUnitId == TEXT("DragonKnight"))
	{
		TArray<ETWStatusType> TargetStatuses = {
			ETWStatusType::Damage,
			ETWStatusType::Armor,
			ETWStatusType::AttackSpeed,
			ETWStatusType::MoveSpeed
		};

		UnitSubsystem->ApplyTemporaryMultiplierBuffToFriendlyUnits(
			HeroLocation,
			ResolveHeroSkillDamageRadius(HeroRow),
			PS->PlayerSlot,
			HeroRow->StatMultiplier,
			HeroRow->BuffDuration,
			TargetStatuses,
			true
		);


		ServerHeroSkillCooldownEndTime = GetWorld()->GetTimeSeconds() + HeroRow->SkillCooldown;
		ClientNotifyHeroSkillCooldown(HeroRow->SkillCooldown);
		ClientRefreshSelectedUnitStatusAndUI();
		return;
	}

	if (HeroUnitId == TEXT("Astrologian"))
	{
		const float AreaRadius = ResolveHeroSkillDamageRadius(HeroRow);

		TArray<FMassEntityHandle> EnemyEntities;
		int32 HitUnitCount = 0;
		int32 HitBuildingCount = 0;

		if (UnitSubsystem->GetEnemyEntitiesInRadius(
			InTargetLocation,
			AreaRadius,
			PS->PlayerSlot,
			EnemyEntities))
		{
			for (const FMassEntityHandle& EnemyEntity : EnemyEntities)
			{
				if (UnitSubsystem->TryApplyDamageToEntity(EnemyEntity, SkillDamage))
				{
					HitUnitCount++;
					NotifyAllPlayersUnitDamaged(EnemyEntity);
				}
			}
		}

		for (TActorIterator<ATWBaseBuilding> It(GetWorld()); It; ++It)
		{
			ATWBaseBuilding* Building = *It;
			if (!IsValid(Building))
			{
				continue;
			}

			if (Building->OwnerPlayerSlot == PS->PlayerSlot)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared2D(Building->GetActorLocation(), InTargetLocation);
			if (DistSq > FMath::Square(AreaRadius))
			{
				continue;
			}

			if (TryApplyDamageToBuilding(Building, SkillDamage))
			{
				HitBuildingCount++;
				NotifyAllPlayersBuildingDamaged(Building);
			}
		}

		NotifyAllPlayersHeroSkillFX(HeroUnitId, HeroLocation, InTargetLocation, AreaRadius);

		ServerHeroSkillCooldownEndTime = GetWorld()->GetTimeSeconds() + HeroRow->SkillCooldown;
		ClientNotifyHeroSkillCooldown(HeroRow->SkillCooldown);
		ClientRefreshSelectedUnitStatusAndUI();

		return;
	}

	if (HeroUnitId == TEXT("Markman"))
	{
		FMassEntityHandle TargetEntity;
		ATWBaseBuilding* TargetBuilding = nullptr;
		const float AcquireRadius = ResolveHeroSkillAcquireRadius(HeroRow);
		FVector FinalTargetLocation = InTargetLocation;
		bool bHasTargetEntity = false;
		bool bHasTargetBuilding = false;

		if (UnitSubsystem->FindNearestEnemyEntityAroundLocation(
			InTargetLocation,
			AcquireRadius,
			PS->PlayerSlot,
			TargetEntity))
		{
			bHasTargetEntity = true;
			if (UnitSubsystem->TryApplyDamageToEntity(TargetEntity, SkillDamage))
			{
				NotifyAllPlayersUnitDamaged(TargetEntity);
			}

			FVector TargetEntityLocation = FVector::ZeroVector;
			if (UnitSubsystem->TryGetUnitWorldLocation(TargetEntity, TargetEntityLocation))
			{
				FinalTargetLocation = TargetEntityLocation;
			}
		}
		else
		{
			FTWAttackableBuildingCandidate BuildingCandidate;
			if (UnitSubsystem->FindNearestEnemyBuildingCandidate(InTargetLocation, PS->PlayerSlot, BuildingCandidate, AcquireRadius))
			{
				TargetBuilding = BuildingCandidate.Building.Get();
				if (IsValid(TargetBuilding) && TryApplyDamageToBuilding(TargetBuilding, SkillDamage))
				{
					bHasTargetBuilding = true;
					FinalTargetLocation = BuildingCandidate.TargetLocation;
					NotifyAllPlayersBuildingDamaged(TargetBuilding);
				}
			}
		}

		const FVector ProjectileSpawnLocation = HeroLocation + FVector(0.f, 0.f, HeroRow ? HeroRow->ProjectileSpawnZOffset : 50.f);
		NotifyAllPlayersHeroSkillFX(HeroUnitId, ProjectileSpawnLocation, FinalTargetLocation, 0.f);

		if (!HeroRow->ProjectileClass)
		{
			UE_LOG(LogTWCommand, Error, TEXT("[HeroSkill][Server] ProjectileClass NULL | UnitId=%s"), *HeroUnitId.ToString());
		}

		ServerHeroSkillCooldownEndTime = GetWorld()->GetTimeSeconds() + HeroRow->SkillCooldown;
		ClientNotifyHeroSkillCooldown(HeroRow->SkillCooldown);
		ClientRefreshSelectedUnitStatusAndUI();
		
		return;
	}
}
void ATWPlayerController::BeginHeroSkillTargeting(FName InHeroUnitId)
{
	ClearArmedCommandId();
	
	bHeroSkillTargetingMode = true;
	PendingHeroSkillUnitId = InHeroUnitId;

	const float TargetRadius = ResolveHeroSkillTargetRadius(InHeroUnitId);
	ApplyHeroSkillTargetDecalStyle(InHeroUnitId, TargetRadius);
	UpdateHeroSkillTargetDecal(GetMouseWorldLocation());
	
	UpdateInputOverlayState();
}

void ATWPlayerController::ConfirmHeroSkillTargeting()
{
	if (!bHeroSkillTargetingMode)
	{
		return;
	}

	const FVector TargetLocation = GetMouseWorldLocation();
	const FName HeroUnitId = PendingHeroSkillUnitId;

	if (HeroUnitId.IsNone())
	{
		HideHeroSkillTargetDecal();
		bHeroSkillTargetingMode = false;
		PendingHeroSkillUnitId = NAME_None;
		return;
	}

	ServerUseHeroSkill(TargetLocation, HeroUnitId);

	HideHeroSkillTargetDecal();
	bHeroSkillTargetingMode = false;
	PendingHeroSkillUnitId = NAME_None;
	
	UpdateInputOverlayState();
}

void ATWPlayerController::CancelHeroSkillTargeting()
{
	HideHeroSkillTargetDecal();

	bHeroSkillTargetingMode = false;
	PendingHeroSkillUnitId = NAME_None;
	
	UpdateInputOverlayState();
}
void ATWPlayerController::UpdateHeroSkillTargeting()
{
	if (!bHeroSkillTargetingMode)
	{
		return;
	}

	const FVector TargetLocation = GetMouseWorldLocation();
	UpdateHeroSkillTargetDecal(TargetLocation);
}
ATWHeroUnitBase* ATWPlayerController::GetOwnedHeroUnit() const
{
	const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
	if (!LocalPS || !GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<ATWHeroUnitBase> It(GetWorld()); It; ++It)
	{
		ATWHeroUnitBase* HeroUnit = *It;
		if (!IsValid(HeroUnit))
		{
			continue;
		}

		if (HeroUnit->GetOwnerPlayerSlot() != LocalPS->PlayerSlot)
		{
			continue;
		}

		return HeroUnit;
	}

	return nullptr;
}

float ATWPlayerController::GetHeroSkillRemainingCooldownTime() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	const float Remaining = LocalHeroSkillCooldownEndTime - GetWorld()->GetTimeSeconds();
	return FMath::Max(0.f, Remaining);
}

bool ATWPlayerController::IsHeroSkillReady() const
{
	return GetHeroSkillRemainingCooldownTime() <= KINDA_SMALL_NUMBER;
}

void ATWPlayerController::ClientNotifyHeroSkillCooldown_Implementation(float InDuration)
{
	if (!GetWorld())
	{
		return;
	}

	LocalHeroSkillCooldownEndTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, InDuration);
	RefreshUIBridge();
}

void ATWPlayerController::ClientRefreshSelectedUnitStatusAndUI_Implementation()
{
	RefreshLocalPrimarySelectedUnitStatus();
	RefreshUIBridge();
	RefreshSelectionVisualManager();

	if (UWorld* World = GetWorld())
	{
		FTimerHandle DeferredRefreshHandle;
		World->GetTimerManager().SetTimer(
			DeferredRefreshHandle,
			[this]()
			{
				RefreshLocalPrimarySelectedUnitStatus();
				RefreshUIBridge();
				RefreshSelectionVisualManager();
			},
			0.1f,
			false
		);
	}
}

bool ATWPlayerController::IsOwnedHeroCurrentlySelected() const
{
	FName SelectedHeroUnitId = NAME_None;
	return TryGetSelectedHeroUnitId(SelectedHeroUnitId);
}

bool ATWPlayerController::TryGetSelectedHeroEntityHandle(FMassEntityHandle& OutEntityHandle, FName& OutHeroUnitId) const
{
	OutEntityHandle = FMassEntityHandle();
	OutHeroUnitId = NAME_None;

	if (!HasAuthority())
	{
		return false;
	}

	ATWPlayerState* PS = GetPlayerState<ATWPlayerState>();
	if (!PS)
	{
		return false;
	}

	UMassEntitySubsystem* EntitySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
	if (!EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	for (const FMassEntityHandle& Entity : ServerSelectedEntities)
	{
		if (!EntityManager.IsEntityValid(Entity) || !EntityManager.IsEntityActive(Entity))
		{
			continue;
		}

		const FTWUnitFragment* UnitFrag = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
		if (!UnitFrag)
		{
			continue;
		}

		if (UnitFrag->GetOwner() != PS->PlayerSlot)
		{
			continue;
		}

		const FName UnitId = UnitFrag->GetUnitID();
		if (UnitId == PS->GetSelectedHeroUnitId())
		{
			OutEntityHandle = Entity;
			OutHeroUnitId = UnitId;
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

	TArray<FMassNetworkID> ResolvedSelectedUnitNetIds;
	GetResolvedLocalSelectedUnitNetIds(ResolvedSelectedUnitNetIds);

	PlayerSelectionVisualComponent->RefreshSelectionVisuals(
		ResolvedSelectedUnitNetIds,
		SelectedBuilding,
		LocalSelectedOwnerPlayerSlot
	);
}

void ATWPlayerController::GetResolvedLocalSelectedUnitNetIds(TArray<FMassNetworkID>& OutUnitNetIds) const
{
	OutUnitNetIds.Reset();

	if (HasAuthority() && ServerSelectedEntities.Num() > 0)
	{
		OutUnitNetIds.Reserve(ServerSelectedEntities.Num());

		for (const FMassEntityHandle& EntityHandle : ServerSelectedEntities)
		{
			FName UnitId = NAME_None;
			FMassNetworkID NetId;
			if (TryGetSelectionDataFromEntityHandle(GetWorld(), EntityHandle, UnitId, NetId) && NetId.IsValid())
			{
				OutUnitNetIds.Add(NetId);
			}
		}

		if (OutUnitNetIds.Num() > 0)
		{
			return;
		}
	}

	OutUnitNetIds = ClientSelectedEntities;
}

bool ATWPlayerController::TryGetAuthoritativeSelectionData(
	const FMassEntityHandle& EntityHandle,
	FName& OutUnitId,
	FMassNetworkID& OutNetId,
	int32& OutOwnerPlayerSlot,
	FTWUnitStatus& OutStatus) const
{
	OutOwnerPlayerSlot = INDEX_NONE;
	OutStatus = FTWUnitStatus();
	return TryGetSelectionDataFromEntityHandle(
		GetWorld(),
		EntityHandle,
		OutUnitId,
		OutNetId,
		&OutOwnerPlayerSlot,
		&OutStatus);
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
	LocalPrimarySelectedUnitNetId = FMassNetworkID();
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


void ATWPlayerController::ResolveLocalSelectedUnitIds(TArray<FName>& OutUnitIds) const
{
	OutUnitIds.Empty();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return;
	}

	if (HasAuthority() && ServerSelectedEntities.Num() > 0)
	{
		for (const FMassEntityHandle& EntityHandle : ServerSelectedEntities)
		{
			FName UnitId = NAME_None;
			FMassNetworkID UnusedNetId;
			if (TryGetSelectionDataFromEntityHandle(GetWorld(), EntityHandle, UnitId, UnusedNetId) && !UnitId.IsNone())
			{
				OutUnitIds.Add(UnitId);
			}
		}

		if (OutUnitIds.Num() > 0)
		{
			return;
		}
	}

	for (const FMassNetworkID& NetId : ClientSelectedEntities)
	{
		FName UnitId = NAME_None;
		if (UnitSubsystem->TryGetUnitID(NetId, UnitId) && !UnitId.IsNone())
		{
			OutUnitIds.Add(UnitId);
		}
	}
}

bool ATWPlayerController::TryResolveLocalPrimarySelectedUnitFromClientSelection(FName& OutUnitId, FMassNetworkID& OutPrimaryNetId) const
{
	OutUnitId = NAME_None;
	OutPrimaryNetId = FMassNetworkID();

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return false;
	}

	if (HasAuthority() && ServerSelectedEntities.Num() > 0)
	{
		for (const FMassEntityHandle& EntityHandle : ServerSelectedEntities)
		{
			FName UnitId = NAME_None;
			FMassNetworkID NetId;
			if (!TryGetSelectionDataFromEntityHandle(GetWorld(), EntityHandle, UnitId, NetId) || UnitId.IsNone())
			{
				continue;
			}

			if (LocalPrimarySelectedUnitNetId.IsValid() && NetId.IsValid() && NetId == LocalPrimarySelectedUnitNetId)
			{
				OutUnitId = UnitId;
				OutPrimaryNetId = NetId;
				return true;
			}

			if (!LocalPrimarySelectedUnitId.IsNone() && UnitId == LocalPrimarySelectedUnitId)
			{
				OutUnitId = UnitId;
				OutPrimaryNetId = NetId;
				return true;
			}

			if (OutUnitId.IsNone())
			{
				OutUnitId = UnitId;
				OutPrimaryNetId = NetId;
			}
		}

		if (!OutUnitId.IsNone())
		{
			return true;
		}
	}

	if (LocalPrimarySelectedUnitNetId.IsValid() && ClientSelectedEntities.Contains(LocalPrimarySelectedUnitNetId))
	{
		FName PreservedUnitId = NAME_None;
		if (UnitSubsystem->TryGetUnitID(LocalPrimarySelectedUnitNetId, PreservedUnitId) && !PreservedUnitId.IsNone())
		{
			OutUnitId = PreservedUnitId;
			OutPrimaryNetId = LocalPrimarySelectedUnitNetId;
			return true;
		}
	}

	for (const FMassNetworkID& NetId : ClientSelectedEntities)
	{
		FName UnitId = NAME_None;
		if (!UnitSubsystem->TryGetUnitID(NetId, UnitId) || UnitId.IsNone())
		{
			continue;
		}

		if (!LocalPrimarySelectedUnitId.IsNone() && UnitId == LocalPrimarySelectedUnitId)
		{
			OutUnitId = UnitId;
			OutPrimaryNetId = NetId;
			return true;
		}

		if (OutUnitId.IsNone())
		{
			OutUnitId = UnitId;
			OutPrimaryNetId = NetId;
		}
	}

	return !OutUnitId.IsNone() && OutPrimaryNetId.IsValid();
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
void ATWPlayerController::ForceRefreshSelectionFromHero()
{
	// 기존 hero fallback 강제 보정은 리슨 서버에서 실제 선택 상태를 덮어써서
	// 잘못된 영웅/체력 UI를 만들 수 있으므로 no-op로 유지한다.
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

	const FName PreviousPrimaryUnitId = LocalPrimarySelectedUnitId;
	const FMassNetworkID PreviousPrimaryNetId = LocalPrimarySelectedUnitNetId;
	const bool bPreviousHadStatus = bHasLocalPrimarySelectedUnitStatus;
	const int32 PreviousSummaryCount = LocalSelectionSummaryItems.Num();
	TArray<FSelectionSummaryItemViewModel> PreviousSummaryItems = LocalSelectionSummaryItems;

	const bool bHasAnyUnitSelection =
		(HasAuthority() && ServerSelectedEntities.Num() > 0) ||
		(ClientSelectedEntities.Num() > 0);

	if (!bHasAnyUnitSelection)
	{
		LocalPrimarySelectedUnitStatus = FTWUnitStatus();
		bHasLocalPrimarySelectedUnitStatus = false;
		LocalPrimarySelectedUnitId = NAME_None;
		LocalPrimarySelectedUnitNetId = FMassNetworkID();
		LocalSelectionSummaryItems.Empty();
		return;
	}

	TArray<FName> UnitIds;
	ResolveLocalSelectedUnitIds(UnitIds);
	if (UnitIds.Num() > 0)
	{
		RebuildLocalSelectionSummary(UnitIds);
	}

	FName ResolvedPrimaryUnitId = NAME_None;
	FMassNetworkID ResolvedPrimaryNetId;
	if (TryResolveLocalPrimarySelectedUnitFromClientSelection(ResolvedPrimaryUnitId, ResolvedPrimaryNetId))
	{
		LocalPrimarySelectedUnitId = ResolvedPrimaryUnitId;
		LocalPrimarySelectedUnitNetId = ResolvedPrimaryNetId;
	}

	if (RefreshLocalPrimarySelectedUnitStatus())
	{
		const bool bPrimaryChanged =
			(PreviousPrimaryUnitId != LocalPrimarySelectedUnitId) ||
			(PreviousPrimaryNetId != LocalPrimarySelectedUnitNetId);
		const bool bStatusBecameValid = (!bPreviousHadStatus && bHasLocalPrimarySelectedUnitStatus);
		const bool bSummaryChanged =
			(PreviousSummaryCount != LocalSelectionSummaryItems.Num()) ||
			!AreSelectionSummaryItemsEqual(PreviousSummaryItems, LocalSelectionSummaryItems);

		if (bPrimaryChanged || bStatusBecameValid || bSummaryChanged)
		{
			RefreshDynamicMappingContexts();
			RefreshUIBridge();
			RefreshSelectionVisualManager();
		}
		return;
	}
	
	TArray<FName> RemainingUnitIds;
	ResolveLocalSelectedUnitIds(RemainingUnitIds);

	if (RemainingUnitIds.Num() <= 0)
	{
		ClearLocalSelectionCache();
		RefreshDynamicMappingContexts();
		RefreshUIBridge();
		RefreshSelectionVisualManager();
		return;
	}

	// 선택은 살아 있지만 복제 동기화가 잠깐 늦는 프레임에서는
	// 직전 유효 UnitId/Status를 유지한다.
}


bool ATWPlayerController::RefreshLocalPrimarySelectedUnitStatus()
{
	const bool bHasAnyUnitSelection =
		(HasAuthority() && ServerSelectedEntities.Num() > 0) ||
		(ClientSelectedEntities.Num() > 0);

	if (!bHasAnyUnitSelection)
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

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return false;
	}

	if (HasAuthority() && ServerSelectedEntities.Num() > 0)
	{
		const FMassEntityHandle PrimaryEntityHandle = ServerSelectedEntities[0];
		FName PrimaryUnitId = NAME_None;
		FMassNetworkID PrimaryNetId;
		int32 OwnerSlot = LocalSelectedOwnerPlayerSlot;
		FTWUnitStatus Status;

		if (!TryGetSelectionDataFromEntityHandle(
			World,
			PrimaryEntityHandle,
			PrimaryUnitId,
			PrimaryNetId,
			&OwnerSlot,
			&Status))
		{
			return false;
		}

		LocalPrimarySelectedUnitId = PrimaryUnitId;
		LocalPrimarySelectedUnitNetId = PrimaryNetId;
		LocalSelectedOwnerPlayerSlot = OwnerSlot;
		LocalPrimarySelectedUnitStatus = Status;
		bHasLocalPrimarySelectedUnitStatus = true;
		return true;
	}

	FMassNetworkID PrimaryNetId = LocalPrimarySelectedUnitNetId;
	FName PrimaryUnitId = LocalPrimarySelectedUnitId;
	if (!PrimaryNetId.IsValid() || PrimaryUnitId.IsNone())
	{
		if (!TryResolveLocalPrimarySelectedUnitFromClientSelection(PrimaryUnitId, PrimaryNetId))
		{
			return false;
		}
		LocalPrimarySelectedUnitId = PrimaryUnitId;
		LocalPrimarySelectedUnitNetId = PrimaryNetId;
	}

	int32 OwnerSlot = LocalSelectedOwnerPlayerSlot;
	UnitSubsystem->TryGetUnitOwnerPlayerSlot(PrimaryNetId, OwnerSlot);

	FTWUnitStatus Status = UnitSubsystem->GetUnitCurrentStatus(PrimaryNetId, OwnerSlot);
	float CurrentHP = 0.f;
	if (!UnitSubsystem->TryGetUnitCurrentHP(PrimaryNetId, OwnerSlot, CurrentHP))
	{
		return false;
	}

	Status.SetStatus(ETWStatusType::Health, CurrentHP);
	LocalPrimarySelectedUnitStatus = Status;
	bHasLocalPrimarySelectedUnitStatus = true;
	return true;
}


void ATWPlayerController::ClientApplyUnitSelection_Implementation(
	const TArray<FMassNetworkID>& InNetworkIds,
	float InPrimaryHealth,
	bool bInHasPrimaryHealth,
	int32 InSelectedOwnerPlayerSlot)
{
	if (ShouldRejectIncomingUnitSelectionByFog(InNetworkIds, InSelectedOwnerPlayerSlot))
	{
		ClientClearSelection_Implementation();
		return;
	}
	
	if (bBuildShortcutModeActive)
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
	}
	
	ClearLocalSelectionCache();

	LocalSelectedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
	LocalSelectedUnitCount = InNetworkIds.Num();
	ClientSelectedEntities = InNetworkIds;

	TArray<FName> UnitIds;
	ResolveLocalSelectedUnitIds(UnitIds);
	if (UnitIds.Num() > 0)
	{
		RebuildLocalSelectionSummary(UnitIds);
	}

	FName ResolvedPrimaryUnitId = NAME_None;
	FMassNetworkID ResolvedPrimaryNetId;
	if (TryResolveLocalPrimarySelectedUnitFromClientSelection(ResolvedPrimaryUnitId, ResolvedPrimaryNetId))
	{
		LocalPrimarySelectedUnitId = ResolvedPrimaryUnitId;
		LocalPrimarySelectedUnitNetId = ResolvedPrimaryNetId;
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
	if (ShouldRejectIncomingBuildingSelectionByFog(InBuilding))
	{
		ClientClearSelection_Implementation();
		return;
	}
	
	if (bBuildShortcutModeActive)
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
	}
	
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
				return false;
			}

			const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
			if (!LocalPS)
			{
				return false;
			}

			if (Selected->OwnerPlayerSlot != LocalPS->PlayerSlot)
			{
				return false;
			}

			// 인구 증가 전용
			if (CommandId == TEXT("IncreasePopulation"))
			{
				const bool bValidPopulationBuilding = (Cast<ATWPopulationBuilding>(Selected) != nullptr);
				return bValidPopulationBuilding;
			}

			// 일반 병력 생산 전용
			const bool bValidTroopBuilding = (Cast<ATWTroopSpawnBuilding>(Selected) != nullptr);
			return bValidTroopBuilding;
		}

	case ETWCommandType::Research:
		{
			ATWBaseBuilding* Selected = GetSelectedBuilding();
			if (!IsValid(Selected))
			{
				return false;
			}

			const ATWPlayerState* LocalPS = GetPlayerState<ATWPlayerState>();
			if (!LocalPS)
			{
				return false;
			}

			if (Selected->OwnerPlayerSlot != LocalPS->PlayerSlot)
			{
				return false;
			}

			const bool bValidUpgradeBuilding = (Cast<ATWUpgradeBuilding>(Selected) != nullptr);
			
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
					LogTWCommand,
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
	if (bHeroSkillTargetingMode)
	{
		CancelHeroSkillTargeting();
	}
	
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
	UE_LOG(LogTWCommand, Log, TEXT("[Command] HandleCommandById: %s"), *CommandId.ToString());

	if (CommandId.IsNone())
	{
		return;
	}

	const FUICommandMetaRow* CommandMeta = FindCommandMetaRowFromTable(CommandId);
	if (!CommandMeta)
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[Command] CommandMeta not found: %s"), *CommandId.ToString());
		return;
	}
	if (CommandId == TEXT("HeroSkill"))
	{
		FName SelectedHeroUnitId = NAME_None;
		if (!TryGetSelectedHeroUnitId(SelectedHeroUnitId))
		{
			return;
		}

		if (!IsHeroSkillReady())
		{
			return;
		}

		// 타겟팅 필요한 영웅
		if (SelectedHeroUnitId == TEXT("Markman") ||
			SelectedHeroUnitId == TEXT("Astrologian"))
		{
			BeginHeroSkillTargeting(SelectedHeroUnitId);
			return;
		}

		// 즉시 시전 (DragonKnight)
		ServerUseHeroSkill(GetMouseWorldLocation(), SelectedHeroUnitId);
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
		LogTWCommand,
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
		UE_LOG(LogTWCommand, Warning, TEXT("[Command][Server] CommandMeta not found: %s"), *CommandId.ToString());
		return;
	}

	ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		UE_LOG(LogTWCommand, Warning, TEXT("[Command][Server] PlayerState not found: %s"), *CommandId.ToString());
		return;
	}

	switch (CommandMeta->CommandType)
	{
	case ETWCommandType::ProduceUnit:
	{
		ATWBaseBuilding* CurrentSelectedBuilding = GetSelectedBuilding();
		if (!IsValid(CurrentSelectedBuilding))
		{
			UE_LOG(LogTWCommand, Warning, TEXT("[Command][Server] Produce failed: no selected building"));
			return;
		}

		if (CurrentSelectedBuilding->OwnerPlayerSlot != TWPS->PlayerSlot)
		{
			UE_LOG(
				LogTWCommand,
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
					LogTWCommand,
					Warning,
					TEXT("[Command][Server] Research failed: no selected building | CommandId=%s"),
					*CommandId.ToString()
				);
				return;
			}

			if (CurrentSelectedBuilding->OwnerPlayerSlot != TWPS->PlayerSlot)
			{
				UE_LOG(
					LogTWCommand,
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
					LogTWCommand,
					Warning,
					TEXT("[Command][Server] Research failed: selected building is not ATWUpgradeBuilding | CommandId=%s"),
					*CommandId.ToString()
				);
				return;
			}

			if (CommandMeta->PayloadId.IsNone() || CommandMeta->PayloadId == TEXT("None"))
			{
				UE_LOG(
					LogTWCommand,
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
					LogTWCommand,
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
			LogTWCommand,
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

void ATWPlayerController::Server_RequestDefeat_Implementation()
{
	ATWGameMode* GM = GetWorld()->GetAuthGameMode<ATWGameMode>();
	ATWPlayerState* PS = GetPlayerState<ATWPlayerState>();
	if (GM && PS)
	{
		GM->HandlePlayerDefeat(PS->PlayerSlot);
	}
}

void ATWPlayerController::Client_ShowAlertMessage_Implementation(const FString& AlertMessage)
{
	if (AlertWidgetClass)
	{
		UTWAlertWidget* AlertUI = CreateWidget<UTWAlertWidget>(this, AlertWidgetClass);
		
		if (AlertUI)
		{
			AlertUI->SetAlertText(AlertMessage);
			
			AlertUI->AddToViewport();
		}
	}
}

void ATWPlayerController::Client_ShowMenu_Implementation(bool Open)
{
	
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

void ATWPlayerController::OnPlayerStateHeroChanged(FName InHeroUnitId)
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(
		LogTWCommand,
		Log,
		TEXT("[Selection] OnPlayerStateHeroChanged | Player=%s | HeroId=%s | SelectedCount=%d | OwnerSlot=%d"),
		*GetNameSafe(this),
		*InHeroUnitId.ToString(),
		LocalSelectedUnitCount,
		LocalSelectedOwnerPlayerSlot
	);

	if (SelectedBuilding)
	{
		RefreshDynamicMappingContexts();
		RefreshUIBridge();
		RefreshSelectionVisualManager();
		return;
	}

	const ATWPlayerState* PS = GetPlayerState<ATWPlayerState>();
	if (PS && LocalSelectedOwnerPlayerSlot == INDEX_NONE && LocalSelectedUnitCount > 0)
	{
		LocalSelectedOwnerPlayerSlot = PS->PlayerSlot;
	}

	RefreshLocalSelectionRuntimeData();
	RefreshDynamicMappingContexts();
	RefreshUIBridge();
	RefreshSelectionVisualManager();
}
#pragma endregion

#pragma region Input Overlay
void ATWPlayerController::UpdateInputOverlayState()
{
	if (!PlayerUIControllerComponent)
	{
		return;
	}

	if (bHeroSkillTargetingMode)
	{
		PlayerUIControllerComponent->UpdateInputOverlayState(TEXT("HeroSkill"));
	}
	else
	{
		PlayerUIControllerComponent->UpdateInputOverlayState(ArmedCommandId);
	}
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

#pragma region Cheat

void ATWPlayerController::Server_CheatAddResource_Implementation(EResourceType ResourceType, int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (ATWPlayerState* TWPS = GetPlayerState<ATWPlayerState>())
	{
		TWPS->AddResource(ResourceType, Amount);
	}
}

bool ATWPlayerController::Server_CheatAddResource_Validate(EResourceType ResourceType, int32 Amount)
{
	return true;
}


void ATWPlayerController::Server_CheatTimeScale_Implementation(float TimeMultiplier)
{
	if (UWorld* World = GetWorld())
	{
		float SafeMultiplier = FMath::Max(TimeMultiplier, 0.1f);
		UGameplayStatics::SetGlobalTimeDilation(World, SafeMultiplier);
	}
}

bool ATWPlayerController::Server_CheatTimeScale_Validate(float TimeMultiplier)
{
	return true;
}


#pragma endregion
