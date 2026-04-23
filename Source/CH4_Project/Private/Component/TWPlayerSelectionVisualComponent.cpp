#include "Component/TWPlayerSelectionVisualComponent.h"

#include "Core/TWPlayerController.h"
#include "Building/TWBaseBuilding.h"
#include "Mass/TWUnit.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "Selection/TWSelectionVisualActor.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	static bool TryGetUnitVisualActorFromEntityHandle(
		UWorld* World,
		const FMassEntityHandle& EntityHandle,
		const ATWUnit*& OutUnitActor)
	{
		OutUnitActor = nullptr;

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

		const FMassActorFragment* ActorFragment = EntityManager.GetFragmentDataPtr<FMassActorFragment>(EntityHandle);
		if (!ActorFragment || !ActorFragment->IsValid())
		{
			return false;
		}

		OutUnitActor = Cast<ATWUnit>(ActorFragment->Get());
		return IsValid(OutUnitActor);
	}

	static void PrimeRecentCombatUnitSnapshot(
		UWorld* World,
		const FMassNetworkID& UnitNetId,
		int32& InOutOwnerPlayerSlot,
		FVector& InOutCachedWorldLocation,
		float& InOutCachedCurrentHP,
		float& InOutCachedMaxHP,
		float& InOutCachedHealthPercent,
		float DefaultFallbackUnitHPBarHeight)
	{
		if (!World || !UnitNetId.IsValid())
		{
			return;
		}

		UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
		if (!UnitSubsystem)
		{
			return;
		}

		FVector HPBarLocation = FVector::ZeroVector;
		if (UnitSubsystem->TryGetUnitHPBarWorldLocation(UnitNetId, HPBarLocation))
		{
			InOutCachedWorldLocation = HPBarLocation;
		}
		else
		{
			FVector UnitLocation = FVector::ZeroVector;
			if (UnitSubsystem->TryGetUnitVisualLocation(UnitNetId, UnitLocation))
			{
				InOutCachedWorldLocation = UnitLocation + FVector(0.f, 0.f, DefaultFallbackUnitHPBarHeight);
			}
		}

		int32 ResolvedOwnerPlayerSlot = InOutOwnerPlayerSlot;
		if (UnitSubsystem->TryGetUnitOwnerPlayerSlot(UnitNetId, ResolvedOwnerPlayerSlot))
		{
			InOutOwnerPlayerSlot = ResolvedOwnerPlayerSlot;
		}

		float CurrentHP = 0.f;
		if (UnitSubsystem->TryGetUnitCurrentHP(UnitNetId, InOutOwnerPlayerSlot, CurrentHP))
		{
			InOutCachedCurrentHP = CurrentHP;
		}

		float MaxHP = 0.f;
		if (UnitSubsystem->TryGetUnitMaxHP(UnitNetId, InOutOwnerPlayerSlot, MaxHP))
		{
			InOutCachedMaxHP = MaxHP;
		}

		if (InOutCachedMaxHP > KINDA_SMALL_NUMBER)
		{
			if (InOutCachedCurrentHP <= 0.f)
			{
				InOutCachedCurrentHP = InOutCachedMaxHP;
			}

			InOutCachedHealthPercent = FMath::Clamp(InOutCachedCurrentHP / InOutCachedMaxHP, 0.f, 1.f);
		}
	}
}

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
	const FTWSelectedVisualData PreviousPrimaryVisualData = PrimarySelectedVisualData;
	const TArray<FTWUnitRingVisualData> PreviousUnitRingVisuals = SelectedUnitRingVisuals;
	const TArray<FTWBuildingSelectionVisualData> PreviousBuildingVisuals = SelectedBuildingVisuals;
	const TArray<FTWHPBarVisualData> PreviousHPBarVisuals = SelectedHPBarVisuals;

	ClearCachedVisualData();
	bool bBuiltAnyVisual = false;
	const bool bHasAuthoritativeSelection =
		OwnerController &&
		OwnerController->HasAuthority() &&
		OwnerController->GetAuthoritativeSelectedEntityHandles().Num() > 0;

	if (bHasAuthoritativeSelection)
	{
		bBuiltAnyVisual |= BuildSelectedUnitVisualDataFromEntityHandles(InSelectedOwnerPlayerSlot);
	}
	else if (InSelectedUnitNetIds.Num() > 0)
	{
		bBuiltAnyVisual |= BuildSelectedUnitVisualData(InSelectedUnitNetIds, InSelectedOwnerPlayerSlot);
	}

	if (IsValid(InSelectedBuilding))
	{
		bBuiltAnyVisual |= BuildSelectedBuildingVisualData(InSelectedBuilding);
	}

	if (!PrimarySelectedVisualData.bValid && PreviousPrimaryVisualData.bValid)
	{
		PrimarySelectedVisualData = PreviousPrimaryVisualData;
	}

	if (!bBuiltAnyVisual && (bHasAuthoritativeSelection || InSelectedUnitNetIds.Num() > 0 || IsValid(InSelectedBuilding)))
	{
		PrimarySelectedVisualData = PreviousPrimaryVisualData;
		SelectedUnitRingVisuals = PreviousUnitRingVisuals;
		SelectedBuildingVisuals = PreviousBuildingVisuals;
		SelectedHPBarVisuals = PreviousHPBarVisuals;
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

	const FName PreferredPrimaryUnitId = OwnerController->GetLocalPrimarySelectedUnitId();
	const FMassNetworkID PreferredPrimaryNetId = OwnerController->GetLocalPrimarySelectedUnitNetId();
	bool bAnyBuilt = false;

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

		int32 ResolvedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
		UnitSubsystem->TryGetUnitOwnerPlayerSlot(NetId, ResolvedOwnerPlayerSlot);

		float CurrentHP = 0.f;
		float MaxHP = 0.f;
		const bool bIsPrimaryUnit =
			(PreferredPrimaryNetId.IsValid() && NetId == PreferredPrimaryNetId);
		bool bHasCurrentHP = UnitSubsystem->TryGetUnitCurrentHP(NetId, ResolvedOwnerPlayerSlot, CurrentHP);
		const bool bHasMaxHP = UnitSubsystem->TryGetUnitMaxHP(NetId, ResolvedOwnerPlayerSlot, MaxHP);
		if (bIsPrimaryUnit && OwnerController->HasLocalPrimarySelectedUnitStatus())
		{
			CurrentHP = OwnerController->GetLocalPrimarySelectedUnitStatus().GetStatus(ETWStatusType::Health);
			bHasCurrentHP = true;
		}

		if (!bHasCurrentHP || !bHasMaxHP || MaxHP <= KINDA_SMALL_NUMBER)
		{
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

		FName CurrentUnitId = NAME_None;
		UnitSubsystem->TryGetUnitID(NetId, CurrentUnitId);
		const bool bShouldBecomePrimary =
			!PrimarySelectedVisualData.bValid &&
			(
				bIsPrimaryUnit ||
				(!PreferredPrimaryNetId.IsValid() && InSelectedUnitNetIds.Num() == 1) ||
				(!PreferredPrimaryUnitId.IsNone() && CurrentUnitId == PreferredPrimaryUnitId)
			);

		if (bShouldBecomePrimary)
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
		}

		bAnyBuilt = true;
	}

	return bAnyBuilt;
}

bool UTWPlayerSelectionVisualComponent::BuildSelectedUnitVisualDataFromEntityHandles(int32 InSelectedOwnerPlayerSlot)
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

	const TArray<FMassEntityHandle>& SelectedEntities = OwnerController->GetAuthoritativeSelectedEntityHandles();
	if (SelectedEntities.Num() <= 0)
	{
		return false;
	}

	SelectedUnitRingVisuals.Reserve(SelectedEntities.Num());
	SelectedHPBarVisuals.Reserve(SelectedEntities.Num());

	const FName PreferredPrimaryUnitId = OwnerController->GetLocalPrimarySelectedUnitId();
	const FMassNetworkID PreferredPrimaryNetId = OwnerController->GetLocalPrimarySelectedUnitNetId();
	bool bAnyBuilt = false;

	for (const FMassEntityHandle& EntityHandle : SelectedEntities)
	{
		FName UnitId = NAME_None;
		FMassNetworkID NetId;
		int32 ResolvedOwnerPlayerSlot = InSelectedOwnerPlayerSlot;
		FTWUnitStatus RuntimeStatus;
		if (!OwnerController->TryGetAuthoritativeSelectionData(
			EntityHandle,
			UnitId,
			NetId,
			ResolvedOwnerPlayerSlot,
			RuntimeStatus))
		{
			continue;
		}

		FVector RingLocation = FVector::ZeroVector;
		if ((!NetId.IsValid() || !UnitSubsystem->TryGetUnitVisualLocation(NetId, RingLocation)) &&
			!UnitSubsystem->TryGetUnitWorldLocation(EntityHandle, RingLocation))
		{
			continue;
		}

		FTWUnitRingVisualData RingData;
		RingData.UnitNetId = NetId;
		RingData.RingWorldLocation = RingLocation;
		RingData.RingRadius = DefaultUnitSelectionCircleRadius;

		if (NetId.IsValid())
		{
			FTWUnitSelectionVisualStyle Style;
			if (UnitSubsystem->TryGetUnitSelectionVisualStyle(NetId, Style))
			{
				RingData.RingRadius = Style.SelectionCircleRadius;
			}
		}

		SelectedUnitRingVisuals.Add(RingData);

		FVector HPBarLocation = FVector::ZeroVector;
		const ATWUnit* UnitActor = nullptr;
		if (TryGetUnitVisualActorFromEntityHandle(World, EntityHandle, UnitActor) && IsValid(UnitActor))
		{
			HPBarLocation = UnitActor->GetHPBarAnchorWorldLocation();
		}
		else if (!NetId.IsValid() || !UnitSubsystem->TryGetUnitHPBarWorldLocation(NetId, HPBarLocation))
		{
			HPBarLocation = RingLocation + FVector(0.f, 0.f, DefaultFallbackUnitHPBarHeight);
		}

		float CurrentHP = RuntimeStatus.GetStatus(ETWStatusType::Health);
		bool bHasCurrentHP = true;
		if (PreferredPrimaryNetId.IsValid() &&
			NetId == PreferredPrimaryNetId &&
			OwnerController->HasLocalPrimarySelectedUnitStatus())
		{
			CurrentHP = OwnerController->GetLocalPrimarySelectedUnitStatus().GetStatus(ETWStatusType::Health);
			bHasCurrentHP = true;
		}

		FTWUnitStatus DefaultStatus = UnitSubsystem->GetUnitDefaultStatus(UnitId, ResolvedOwnerPlayerSlot);
		const float MaxHP = DefaultStatus.GetStatus(ETWStatusType::Health);
		if (!bHasCurrentHP || MaxHP <= KINDA_SMALL_NUMBER)
		{
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

		const bool bShouldBecomePrimary =
			!PrimarySelectedVisualData.bValid &&
			(
				(PreferredPrimaryNetId.IsValid() && NetId.IsValid() && NetId == PreferredPrimaryNetId) ||
				(!PreferredPrimaryNetId.IsValid() && SelectedEntities.Num() == 1) ||
				(!PreferredPrimaryUnitId.IsNone() && UnitId == PreferredPrimaryUnitId)
			);

		if (bShouldBecomePrimary)
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
		if (!Item.bIsBuilding && Item.UnitNetId == InUnitNetId)
		{
			Item.OwnerPlayerSlot = InOwnerPlayerSlot;
			Item.RemainingTime = SafeVisibleTime;
			Item.bValid = true;
			Item.bIsBuilding = false;
			PrimeRecentCombatUnitSnapshot(
				GetWorld(),
				Item.UnitNetId,
				Item.OwnerPlayerSlot,
				Item.CachedWorldLocation,
				Item.CachedCurrentHP,
				Item.CachedMaxHP,
				Item.CachedHealthPercent,
				DefaultFallbackUnitHPBarHeight
			);
			return;
		}
	}

	FTWRecentCombatHPBarData NewItem;
	NewItem.bValid = true;
	NewItem.bIsBuilding = false;
	NewItem.UnitNetId = InUnitNetId;
	NewItem.Building = nullptr;
	NewItem.OwnerPlayerSlot = InOwnerPlayerSlot;
	NewItem.RemainingTime = SafeVisibleTime;
	PrimeRecentCombatUnitSnapshot(
		GetWorld(),
		NewItem.UnitNetId,
		NewItem.OwnerPlayerSlot,
		NewItem.CachedWorldLocation,
		NewItem.CachedCurrentHP,
		NewItem.CachedMaxHP,
		NewItem.CachedHealthPercent,
		DefaultFallbackUnitHPBarHeight
	);
	RecentCombatHPBars.Add(NewItem);
}

void UTWPlayerSelectionVisualComponent::NotifyRecentCombatBuildingDamaged(
	ATWBaseBuilding* InBuilding,
	float InVisibleTime
)
{
	if (!IsValid(InBuilding))
	{
		return;
	}

	const float SafeVisibleTime =
		(InVisibleTime > 0.f)
		? InVisibleTime
		: RecentCombatHPBarVisibleTime;

	for (FTWRecentCombatHPBarData& Item : RecentCombatHPBars)
	{
		if (Item.bIsBuilding && Item.Building.Get() == InBuilding)
		{
			Item.OwnerPlayerSlot = InBuilding->OwnerPlayerSlot;
			Item.RemainingTime = SafeVisibleTime;
			Item.bValid = true;
			return;
		}
	}

	FTWRecentCombatHPBarData NewItem;
	NewItem.bValid = true;
	NewItem.bIsBuilding = true;
	NewItem.UnitNetId = FMassNetworkID();
	NewItem.Building = InBuilding;
	NewItem.OwnerPlayerSlot = InBuilding->OwnerPlayerSlot;
	NewItem.RemainingTime = SafeVisibleTime;
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
		const FTWRecentCombatHPBarData& Item = RecentCombatHPBars[Index];

		const bool bInvalidBuildingRef =
			Item.bIsBuilding && !Item.Building.IsValid();

		if (!Item.bValid || Item.RemainingTime <= 0.f || bInvalidBuildingRef)
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
	TArray<FMassNetworkID> InvalidUnitNetIds;
	TArray<TWeakObjectPtr<ATWBaseBuilding>> InvalidBuildings;

	for (FTWRecentCombatHPBarData& Item : RecentCombatHPBars)
	{
		if (AddedCount >= MaxRecentCombatHPBarCount)
		{
			break;
		}

		if (Item.bIsBuilding)
		{
			ATWBaseBuilding* Building = Item.Building.Get();
			if (!IsValid(Building))
			{
				InvalidBuildings.Add(Item.Building);
				continue;
			}

			const FVector HPBarLocation = Building->GetSelectionHPBarWorldLocation();
			const float CurrentHP = Building->GetCurrentHP();
			const float MaxHP = Building->GetMaxHP();

			if (MaxHP <= KINDA_SMALL_NUMBER)
			{
				InvalidBuildings.Add(Item.Building);
				continue;
			}

			Item.OwnerPlayerSlot = Building->OwnerPlayerSlot;
			Item.CachedWorldLocation = HPBarLocation;
			Item.CachedCurrentHP = CurrentHP;
			Item.CachedMaxHP = MaxHP;
			Item.CachedHealthPercent = FMath::Clamp(CurrentHP / MaxHP, 0.f, 1.f);

			FTWHPBarVisualData HPBarData;
			HPBarData.bValid = true;
			HPBarData.bIsBuilding = true;
			HPBarData.OwnerPlayerSlot = Item.OwnerPlayerSlot;
			HPBarData.WorldLocation = Item.CachedWorldLocation;
			HPBarData.CurrentHP = Item.CachedCurrentHP;
			HPBarData.MaxHP = Item.CachedMaxHP;
			HPBarData.HealthPercent = Item.CachedHealthPercent;
			HPBarData.WorldScale = HPBarWorldScale;

			RecentCombatHPBarVisuals.Add(HPBarData);
			++AddedCount;
			continue;
		}

		FVector HPBarLocation = FVector::ZeroVector;
		if (!UnitSubsystem->TryGetUnitHPBarWorldLocation(Item.UnitNetId, HPBarLocation))
		{
			FVector UnitLocation = FVector::ZeroVector;
			if (!UnitSubsystem->TryGetUnitVisualLocation(Item.UnitNetId, UnitLocation))
			{
				if (!Item.CachedWorldLocation.IsNearlyZero())
				{
					HPBarLocation = Item.CachedWorldLocation;
				}
				else
				{
					InvalidUnitNetIds.Add(Item.UnitNetId);
					continue;
				}
			}
			else
			{
				HPBarLocation = UnitLocation + FVector(0.f, 0.f, DefaultFallbackUnitHPBarHeight);
			}
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

		if ((!bHasCurrentHP || !bHasMaxHP || MaxHP <= KINDA_SMALL_NUMBER) &&
			Item.CachedMaxHP > KINDA_SMALL_NUMBER)
		{
			if (!bHasCurrentHP)
			{
				CurrentHP = Item.CachedCurrentHP;
			}

			if (!bHasMaxHP || MaxHP <= KINDA_SMALL_NUMBER)
			{
				MaxHP = Item.CachedMaxHP;
			}
		}

		if (MaxHP > KINDA_SMALL_NUMBER && !bHasCurrentHP && CurrentHP <= 0.f)
		{
			CurrentHP = MaxHP;
		}

		if (MaxHP <= KINDA_SMALL_NUMBER)
		{
			InvalidUnitNetIds.Add(Item.UnitNetId);
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

	if (InvalidUnitNetIds.Num() > 0 || InvalidBuildings.Num() > 0)
	{
		RecentCombatHPBars.RemoveAll(
			[&InvalidUnitNetIds, &InvalidBuildings](const FTWRecentCombatHPBarData& Item)
			{
				if (Item.bIsBuilding)
				{
					for (const TWeakObjectPtr<ATWBaseBuilding>& InvalidBuilding : InvalidBuildings)
					{
						if (Item.Building == InvalidBuilding)
						{
							return true;
						}
					}
					return false;
				}

				return InvalidUnitNetIds.Contains(Item.UnitNetId);
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
