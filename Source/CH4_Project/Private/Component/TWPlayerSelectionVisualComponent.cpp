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

	RecentCombatHPBars.Empty();
	RecentCombatHPBarVisuals.Empty();

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

	TickRecentCombatHPBars(DeltaSeconds);
	BuildRecentCombatHPBarVisualData();

	const bool bHasSelectionVisual =
		(SelectedUnitRingVisuals.Num() > 0) ||
		(SelectedBuildingVisuals.Num() > 0) ||
		(SelectedHPBarVisuals.Num() > 0);

	const bool bHasRecentCombatVisual = (RecentCombatHPBarVisuals.Num() > 0);

	if (!bHasSelectionVisual && !bHasRecentCombatVisual)
	{
		HideAllRenderers();
		return;
	}

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
	RecentCombatHPBarVisuals.Empty();
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

	TArray<FMassNetworkID> InvalidNetIds;

	bool bAnyBuilt = false;
	bool bPrimaryBuilt = false;

	for (const FMassNetworkID& NetId : InSelectedUnitNetIds)
	{
		FVector RingLocation = FVector::ZeroVector;
		if (!UnitSubsystem->TryGetUnitVisualLocation(NetId, RingLocation))
		{
			InvalidNetIds.Add(NetId);
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

		int32 ResolvedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
		if (!UnitSubsystem->TryGetUnitOwnerPlayerSlot(NetId, ResolvedOwnerPlayerSlot))
		{
			ResolvedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
		}

		float CurrentHP = 0.f;
		float MaxHP = 0.f;

		const bool bHasCurrentHP = UnitSubsystem->TryGetUnitCurrentHP(NetId, ResolvedOwnerPlayerSlot, CurrentHP);
		const bool bHasMaxHP = UnitSubsystem->TryGetUnitMaxHP(NetId, ResolvedOwnerPlayerSlot, MaxHP);

		if (!bHasCurrentHP || !bHasMaxHP || MaxHP <= KINDA_SMALL_NUMBER)
		{
			InvalidNetIds.Add(NetId);
			continue;
		}

		FTWHPBarVisualData HPBarData;
		HPBarData.bValid = true;
		HPBarData.bIsBuilding = false;
		HPBarData.OwnerPlayerSlot = ResolvedOwnerPlayerSlot;
		HPBarData.WorldLocation = HPBarLocation;
		HPBarData.CurrentHP = CurrentHP;
		HPBarData.MaxHP = MaxHP;
		HPBarData.HealthPercent = FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f);
		HPBarData.WorldScale = HPBarWorldScale;

		SelectedHPBarVisuals.Add(HPBarData);

		if (!bPrimaryBuilt)
		{
			PrimarySelectedVisualData.bValid = true;
			PrimarySelectedVisualData.bIsBuilding = false;
			PrimarySelectedVisualData.OwnerPlayerSlot = ResolvedOwnerPlayerSlot;
			PrimarySelectedVisualData.SelectionWorldLocation = RingLocation;
			PrimarySelectedVisualData.HPBarWorldLocation = HPBarLocation;
			PrimarySelectedVisualData.CurrentHP = CurrentHP;
			PrimarySelectedVisualData.MaxHP = MaxHP;
			PrimarySelectedVisualData.HealthPercent = HPBarData.HealthPercent;
			PrimarySelectedVisualData.SelectionRadius = RingData.RingRadius;
			PrimarySelectedVisualData.BuildingHalfExtentXY = FVector2D::ZeroVector;
			PrimarySelectedVisualData.BuildingZOffset = 0.f;
			bPrimaryBuilt = true;
		}

		bAnyBuilt = true;
	}

	if (InvalidNetIds.Num() > 0)
	{
		LastSelectedUnitNetIds.RemoveAll(
			[&InvalidNetIds](const FMassNetworkID& Id)
			{
				return InvalidNetIds.Contains(Id);
			}
		);
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

void UTWPlayerSelectionVisualComponent::NotifyRecentCombatUnitDamaged(
	const FMassNetworkID& InUnitNetId,
	int32 InOwnerPlayerSlot,
	float InVisibleTime
)
{
	if (!InUnitNetId.IsValid())
	{
		return;
	}

	const float SafeVisibleTime =
		(InVisibleTime > 0.f)
		? InVisibleTime
		: RecentCombatHPBarVisibleTime;

	for (FTWRecentCombatHPBarData& Item : RecentCombatHPBars)
	{
		if (Item.UnitNetId == InUnitNetId)
		{
			Item.OwnerPlayerSlot = InOwnerPlayerSlot;
			Item.RemainingTime = SafeVisibleTime;
			Item.bValid = true;
			return;
		}
	}

	FTWRecentCombatHPBarData NewItem;
	NewItem.UnitNetId = InUnitNetId;
	NewItem.OwnerPlayerSlot = InOwnerPlayerSlot;
	NewItem.RemainingTime = SafeVisibleTime;
	NewItem.bValid = true;
	RecentCombatHPBars.Add(NewItem);
}

void UTWPlayerSelectionVisualComponent::TickRecentCombatHPBars(float DeltaSeconds)
{
	for (FTWRecentCombatHPBarData& Item : RecentCombatHPBars)
	{
		Item.RemainingTime -= DeltaSeconds;
	}

	RemoveExpiredRecentCombatHPBars();
}

void UTWPlayerSelectionVisualComponent::RemoveExpiredRecentCombatHPBars()
{
	for (int32 Index = RecentCombatHPBars.Num() - 1; Index >= 0; --Index)
	{
		if (!RecentCombatHPBars[Index].bValid || RecentCombatHPBars[Index].RemainingTime <= 0.f)
		{
			RecentCombatHPBars.RemoveAt(Index);
		}
	}
}

void UTWPlayerSelectionVisualComponent::BuildRecentCombatHPBarVisualData()
{
	if (!OwnerController)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return;
	}

	if (RecentCombatHPBars.Num() <= 0)
	{
		return;
	}

	RecentCombatHPBarVisuals.Reserve(FMath::Min(RecentCombatHPBars.Num(), MaxRecentCombatHPBarCount));

	int32 AddedCount = 0;
	TArray<FMassNetworkID> InvalidNetIds;

	for (FTWRecentCombatHPBarData& Item : RecentCombatHPBars)
	{
		if (AddedCount >= MaxRecentCombatHPBarCount)
		{
			break;
		}

		FVector HPBarLocation = FVector::ZeroVector;
		if (!UnitSubsystem->TryGetUnitHPBarWorldLocation(Item.UnitNetId, HPBarLocation))
		{
			FVector UnitLocation = FVector::ZeroVector;
			if (!UnitSubsystem->TryGetUnitVisualLocation(Item.UnitNetId, UnitLocation))
			{
				InvalidNetIds.Add(Item.UnitNetId);
				continue;
			}

			HPBarLocation = UnitLocation + FVector(0.f, 0.f, DefaultFallbackUnitHPBarHeight);
		}

		int32 ResolvedOwnerPlayerSlot = Item.OwnerPlayerSlot;
		if (!UnitSubsystem->TryGetUnitOwnerPlayerSlot(Item.UnitNetId, ResolvedOwnerPlayerSlot))
		{
			ResolvedOwnerPlayerSlot = Item.OwnerPlayerSlot;
		}

		float CurrentHP = 0.f;
		float MaxHP = 0.f;

		const bool bHasCurrentHP =
			UnitSubsystem->TryGetUnitCurrentHP(Item.UnitNetId, ResolvedOwnerPlayerSlot, CurrentHP);

		const bool bHasMaxHP =
			UnitSubsystem->TryGetUnitMaxHP(Item.UnitNetId, ResolvedOwnerPlayerSlot, MaxHP);

		if (!bHasCurrentHP || !bHasMaxHP || MaxHP <= KINDA_SMALL_NUMBER)
		{
			InvalidNetIds.Add(Item.UnitNetId);
			continue;
		}

		Item.OwnerPlayerSlot = ResolvedOwnerPlayerSlot;
		Item.CachedWorldLocation = HPBarLocation;
		Item.CachedCurrentHP = CurrentHP;
		Item.CachedMaxHP = MaxHP;
		Item.CachedHealthPercent = FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f);

		FTWHPBarVisualData HPBarData;
		HPBarData.bValid = true;
		HPBarData.bIsBuilding = false;
		HPBarData.OwnerPlayerSlot = ResolvedOwnerPlayerSlot;
		HPBarData.WorldLocation = Item.CachedWorldLocation;
		HPBarData.CurrentHP = Item.CachedCurrentHP;
		HPBarData.MaxHP = Item.CachedMaxHP;
		HPBarData.HealthPercent = Item.CachedHealthPercent;
		HPBarData.WorldScale = HPBarWorldScale;

		RecentCombatHPBarVisuals.Add(HPBarData);
		++AddedCount;
	}

	if (InvalidNetIds.Num() > 0)
	{
		RecentCombatHPBars.RemoveAll(
			[&InvalidNetIds](const FTWRecentCombatHPBarData& Item)
			{
				return InvalidNetIds.Contains(Item.UnitNetId);
			}
		);
	}
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
		SelectedHPBarVisuals,
		RecentCombatHPBarVisuals
	);
	SelectionVisualActor->SyncVisuals();
}