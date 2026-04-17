#include "Component/TWPlayerSelectionVisualComponent.h"

#include "Core/TWPlayerController.h"
#include "Building/TWBaseBuilding.h"
#include "Selection/TWSelectionVisualActor.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

UTWPlayerSelectionVisualComponent::UTWPlayerSelectionVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTWPlayerSelectionVisualComponent::InitializeVisuals(ATWPlayerController* InOwnerController)
{
	if (bVisualsInitialized)
	{
		return;
	}

	OwnerController = InOwnerController;

	if (!OwnerController || !OwnerController->IsLocalController())
	{
		return;
	}

	InitializeRenderBackend();
	HideAllRenderers();

	bVisualsInitialized = true;

	UE_LOG(LogTemp, Warning, TEXT("[SelectionVisualComponent] InitializeVisuals completed."));
}

void UTWPlayerSelectionVisualComponent::RefreshSelectionVisuals(
	const TArray<FMassNetworkID>& InSelectedUnitNetIds,
	ATWBaseBuilding* InSelectedBuilding,
	int32 InSelectedOwnerPlayerSlot
)
{
	LastSelectedUnitNetIds = InSelectedUnitNetIds;
	LastSelectedBuilding = InSelectedBuilding;
	LastSelectedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
}

void UTWPlayerSelectionVisualComponent::ClearSelectionVisuals()
{
	LastSelectedUnitNetIds.Empty();
	LastSelectedBuilding = nullptr;
	LastSelectedOwnerPlayerSlot = INDEX_NONE;

	ClearCachedVisualData();
	HideAllRenderers();
}

void UTWPlayerSelectionVisualComponent::TickVisuals(float DeltaSeconds)
{
	if (!bVisualsInitialized || !OwnerController || !OwnerController->IsLocalController())
	{
		return;
	}

	UpdateSelectionVisualData(
		LastSelectedUnitNetIds,
		LastSelectedBuilding.Get(),
		LastSelectedOwnerPlayerSlot
	);

	SyncRenderBackendFromCachedData();
}

void UTWPlayerSelectionVisualComponent::UpdateSelectionVisualData(
	const TArray<FMassNetworkID>& InSelectedUnitNetIds,
	ATWBaseBuilding* InSelectedBuilding,
	int32 InSelectedOwnerPlayerSlot
)
{
	ClearCachedVisualData();

	if (InSelectedUnitNetIds.Num() > 0)
	{
		BuildSelectedUnitVisualData(InSelectedUnitNetIds, InSelectedOwnerPlayerSlot);
	}

	if (IsValid(InSelectedBuilding))
	{
		BuildSelectedBuildingVisualData(InSelectedBuilding);
	}
}

void UTWPlayerSelectionVisualComponent::ClearCachedVisualData()
{
	PrimarySelectedVisualData.Reset();
	SelectedUnitRingVisuals.Empty();
	SelectedBuildingVisuals.Empty();
	SelectedHPBarVisuals.Empty();
}

bool UTWPlayerSelectionVisualComponent::BuildSelectedUnitVisualData(
	const TArray<FMassNetworkID>& InSelectedUnitNetIds,
	int32 InSelectedOwnerPlayerSlot
)
{
	if (!OwnerController)
	{
		return false;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return false;
	}

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return false;
	}

	SelectedUnitRingVisuals.Reserve(InSelectedUnitNetIds.Num());
	SelectedHPBarVisuals.Reserve(InSelectedUnitNetIds.Num());

	bool bAnyBuilt = false;
	bool bPrimaryBuilt = false;

	for (const FMassNetworkID& NetId : InSelectedUnitNetIds)
	{
		FVector RingLocation = FVector::ZeroVector;
		if (!UnitSubsystem->TryGetUnitVisualLocation(NetId, RingLocation))
		{
			continue;
		}

		FTWUnitRingVisualData RingData;
		RingData.UnitNetId = NetId;
		RingData.RingWorldLocation = RingLocation;
		RingData.RingRadius = DefaultUnitSelectionCircleRadius;

		FTWUnitSelectionVisualStyle Style;
		if (UnitSubsystem->TryGetUnitSelectionVisualStyle(NetId, Style))
		{
			RingData.RingRadius = Style.SelectionCircleRadius;
		}

		SelectedUnitRingVisuals.Add(RingData);

		FVector HPBarLocation = FVector::ZeroVector;
		if (!UnitSubsystem->TryGetUnitHPBarWorldLocation(NetId, HPBarLocation))
		{
			HPBarLocation = RingLocation + FVector(0.f, 0.f, DefaultFallbackUnitHPBarHeight);
		}

		float CurrentHP = 0.f;
		float MaxHP = 0.f;

		const bool bHasCurrentHP = UnitSubsystem->TryGetUnitCurrentHP(NetId, InSelectedOwnerPlayerSlot, CurrentHP);
		const bool bHasMaxHP = UnitSubsystem->TryGetUnitMaxHP(NetId, InSelectedOwnerPlayerSlot, MaxHP);

		FTWHPBarVisualData HPBarData;
		HPBarData.bValid = true;
		HPBarData.bIsBuilding = false;
		HPBarData.OwnerPlayerSlot = InSelectedOwnerPlayerSlot;
		HPBarData.WorldLocation = HPBarLocation;
		HPBarData.CurrentHP = bHasCurrentHP ? CurrentHP : 0.f;
		HPBarData.MaxHP = bHasMaxHP ? MaxHP : 0.f;
		HPBarData.HealthPercent =
			(HPBarData.MaxHP > KINDA_SMALL_NUMBER)
			? FMath::Clamp(HPBarData.CurrentHP / HPBarData.MaxHP, 0.f, 1.f)
			: 0.f;
		HPBarData.WorldScale = HPBarWorldScale;

		SelectedHPBarVisuals.Add(HPBarData);

		if (!bPrimaryBuilt)
		{
			PrimarySelectedVisualData.bValid = true;
			PrimarySelectedVisualData.bIsBuilding = false;
			PrimarySelectedVisualData.OwnerPlayerSlot = InSelectedOwnerPlayerSlot;
			PrimarySelectedVisualData.SelectionWorldLocation = RingLocation;
			PrimarySelectedVisualData.HPBarWorldLocation = HPBarLocation;
			PrimarySelectedVisualData.CurrentHP = HPBarData.CurrentHP;
			PrimarySelectedVisualData.MaxHP = HPBarData.MaxHP;
			PrimarySelectedVisualData.HealthPercent = HPBarData.HealthPercent;
			PrimarySelectedVisualData.SelectionRadius = RingData.RingRadius;
			PrimarySelectedVisualData.BuildingHalfExtentXY = FVector2D::ZeroVector;
			PrimarySelectedVisualData.BuildingZOffset = 0.f;

			bPrimaryBuilt = true;
		}

		bAnyBuilt = true;
	}

	return bAnyBuilt;
}

bool UTWPlayerSelectionVisualComponent::BuildSelectedBuildingVisualData(ATWBaseBuilding* InSelectedBuilding)
{
	if (!IsValid(InSelectedBuilding))
	{
		return false;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	InSelectedBuilding->GetActorBounds(false, Origin, Extent);

	// 바닥 기준 + 살짝 띄움(에디터에서 조정 가능)
	const FVector SelectionLocation =
		Origin - FVector(0.f, 0.f, Extent.Z) + FVector(0.f, 0.f, BuildingGroundLift);

	const FVector HPBarLocation = InSelectedBuilding->GetSelectionHPBarWorldLocation();

	const float CurrentHP = InSelectedBuilding->GetCurrentHP();
	const float MaxHP = InSelectedBuilding->GetMaxHP();

	FTWBuildingSelectionVisualData BuildingVisualData;
	BuildingVisualData.Building = InSelectedBuilding;
	BuildingVisualData.OwnerPlayerSlot = InSelectedBuilding->OwnerPlayerSlot;
	BuildingVisualData.SelectionWorldLocation = SelectionLocation;
	BuildingVisualData.BuildingHalfExtentXY =
		InSelectedBuilding->GetSelectionRectangleHalfExtentXY(DefaultBuildingSelectionPadding);

	// 이미 바닥 기준 위치를 맞췄으므로 추가 ZOffset은 0
	BuildingVisualData.BuildingZOffset = 0.f;

	SelectedBuildingVisuals.Add(BuildingVisualData);

	FTWHPBarVisualData HPBarData;
	HPBarData.bValid = true;
	HPBarData.bIsBuilding = true;
	HPBarData.OwnerPlayerSlot = InSelectedBuilding->OwnerPlayerSlot;
	HPBarData.WorldLocation = HPBarLocation;
	HPBarData.CurrentHP = CurrentHP;
	HPBarData.MaxHP = MaxHP;
	HPBarData.HealthPercent =
		(MaxHP > KINDA_SMALL_NUMBER)
		? FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f)
		: 0.f;
	HPBarData.WorldScale = HPBarWorldScale;

	SelectedHPBarVisuals.Add(HPBarData);

	if (!PrimarySelectedVisualData.bValid)
	{
		PrimarySelectedVisualData.bValid = true;
		PrimarySelectedVisualData.bIsBuilding = true;
		PrimarySelectedVisualData.OwnerPlayerSlot = InSelectedBuilding->OwnerPlayerSlot;
		PrimarySelectedVisualData.SelectionWorldLocation = SelectionLocation;
		PrimarySelectedVisualData.HPBarWorldLocation = HPBarLocation;
		PrimarySelectedVisualData.CurrentHP = CurrentHP;
		PrimarySelectedVisualData.MaxHP = MaxHP;
		PrimarySelectedVisualData.HealthPercent = HPBarData.HealthPercent;
		PrimarySelectedVisualData.SelectionRadius = InSelectedBuilding->GetSelectionVisualRadius();
		PrimarySelectedVisualData.BuildingHalfExtentXY = BuildingVisualData.BuildingHalfExtentXY;
		PrimarySelectedVisualData.BuildingZOffset = 0.f;
	}

	return true;
}

void UTWPlayerSelectionVisualComponent::InitializeRenderBackend()
{
	if (!OwnerController)
	{
		return;
	}

	if (!UnitRingMesh)
	{
		static const TCHAR* ForcedRingMeshPath =
			TEXT("/Game/CH4_Project/UI/Mesh/ST_SelectionRing.ST_SelectionRing");

		UnitRingMesh = LoadObject<UStaticMesh>(nullptr, ForcedRingMeshPath);
	}

	SpawnRenderActorIfNeeded();

	if (SelectionVisualActor)
	{
		SelectionVisualActor->ConfigureRenderAssets(
			UnitRingMesh,
			UnitRingMaterial,
			UnitRingMeshBaseDiameter,
			UnitRingZOffset,
			bRotateUnitRingToGroundPlane,
			UnitRingRotationOffset,
			UnitRingScaleMultiplier,
			BuildingSelectionBoxMesh,
			BuildingSelectionBoxMaterial,
			BuildingSelectionBoxThickness,
			HPBarMesh,
			HPBarMaterial,
			HPBarWorldScale
		);
	}
}

void UTWPlayerSelectionVisualComponent::SpawnRenderActorIfNeeded()
{
	if (SelectionVisualActor || bRenderActorSpawnRequested || !OwnerController)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	bRenderActorSpawnRequested = true;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerController;
	SpawnParams.Instigator = OwnerController->GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	SelectionVisualActor = World->SpawnActor<ATWSelectionVisualActor>(
		ATWSelectionVisualActor::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (SelectionVisualActor)
	{
		SelectionVisualActor->SetActorHiddenInGame(false);
		SelectionVisualActor->SetReplicates(false);
	}
}

void UTWPlayerSelectionVisualComponent::DestroyRenderActorIfNeeded()
{
	if (IsValid(SelectionVisualActor))
	{
		SelectionVisualActor->Destroy();
		SelectionVisualActor = nullptr;
	}

	bRenderActorSpawnRequested = false;
}

void UTWPlayerSelectionVisualComponent::HideAllRenderers()
{
	if (SelectionVisualActor)
	{
		SelectionVisualActor->ClearVisuals();
	}
}

void UTWPlayerSelectionVisualComponent::SyncRenderBackendFromCachedData()
{
	if (!SelectionVisualActor)
	{
		SpawnRenderActorIfNeeded();
	}

	if (!SelectionVisualActor)
	{
		return;
	}

	SelectionVisualActor->SetVisualData(
		PrimarySelectedVisualData,
		SelectedUnitRingVisuals,
		SelectedBuildingVisuals,
		SelectedHPBarVisuals
	);
	SelectionVisualActor->SyncVisuals();
}