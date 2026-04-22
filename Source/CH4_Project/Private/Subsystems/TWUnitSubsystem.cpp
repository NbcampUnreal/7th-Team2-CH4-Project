#include "Subsystems/TWUnitSubsystem.h"

#include "Core/TWPlayerUnitContainer.h"
#include "Core/TWGameState.h"
#include "Core/TWGameMode.h"
#include "Core/TWPlayerState.h"
#include "Core/TWPlayerController.h"
#include "Data/TWUnitTableRowBase.h"
#include "Mass/TWUnit.h"

#include "EngineUtils.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassNavigationSubsystem.h"
#include "MassReplicationSubsystem.h"
#include "MassEntityConfigAsset.h"
#include "MassNavigationFragments.h"
#include "MassSpawner.h"
#include "MassActorSubsystem.h"
#include "Building/TWBaseBuilding.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Replication/BubbleInfo/TWMassClientBubbleInfo.h"
#include "TimerManager.h"
#include "Log/TWLogCategory.h"

namespace
{
	static bool TryResolveActiveEntityManager(
		const UWorld* World,
		FMassEntityManager*& OutEntityManager
	)
	{
		OutEntityManager = nullptr;

		if (!World)
		{
			return false;
		}

		OutEntityManager = UE::Mass::Utils::GetEntityManager(World);
		return OutEntityManager != nullptr;
	}

	static bool IsSafeActiveEntity(
		FMassEntityManager* EntityManager,
		const FMassEntityHandle& Entity
	)
	{
		if (!EntityManager)
		{
			return false;
		}

		if (!Entity.IsSet())
		{
			return false;
		}

		if (!EntityManager->IsEntityValid(Entity))
		{
			return false;
		}

		if (!EntityManager->IsEntityActive(Entity))
		{
			return false;
		}

		return true;
	}
	struct FTWTemporaryBuffEntry
	{
		FMassEntityHandle Entity;
		FTWUnitStatus AppliedDelta;
	};

#ifdef WITH_CLIENT_CODE
	static TMap<FMassNetworkID, FName> GClientUnitIdCache;
	static TMap<FMassNetworkID, int32> GClientOwnerSlotCache;
	static TMap<FMassNetworkID, float> GClientCurrentHPCache;
	static TMap<FMassNetworkID, float> GClientMaxHPCache;
	static TMap<FMassNetworkID, FTWUnitStatus> GClientStatusCache;
#endif

	static void AddStatusDelta(FTWUnitStatus& InOutStatus, const FTWUnitStatus& Delta)
	{
		for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); ++i)
		{
			const ETWStatusType StatusType = static_cast<ETWStatusType>(i);
			const float NewValue =
				InOutStatus.GetStatus(StatusType) + Delta.GetStatus(StatusType);
			InOutStatus.SetStatus(StatusType, NewValue);
		}
	}

	static void SubtractStatusDelta(FTWUnitStatus& InOutStatus, const FTWUnitStatus& Delta)
	{
		for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); ++i)
		{
			const ETWStatusType StatusType = static_cast<ETWStatusType>(i);
			const float NewValue =
				InOutStatus.GetStatus(StatusType) - Delta.GetStatus(StatusType);
			InOutStatus.SetStatus(StatusType, NewValue);
		}
	}
}

UTWUnitSubsystem::UTWUnitSubsystem()
{
}

void UTWUnitSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const FString Path = TEXT("/Game/CH4_Project/Datas/Tables/DT_UnitTableRowBase.DT_UnitTableRowBase");
	UnitTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *Path));

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (IsValid(EntitySubsystem))
	{
		FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
		FindNearestEntityQuery.Initialize(EntityManager.AsShared());
		FindNearestEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
		FindNearestEntityQuery.AddRequirement<FTWUnitFragment>(EMassFragmentAccess::ReadOnly);
	}

	TArray<FTWUnitTableRowBase*> UnitTableRowBases;
	if (UnitTable)
	{
		UnitTable->GetAllRows<FTWUnitTableRowBase>(TEXT("UnitSubsystem::OnWorldBeginPlay"), UnitTableRowBases);
		for (const FTWUnitTableRowBase* UnitTableRowBase : UnitTableRowBases)
		{
			if (!UnitTableRowBase)
			{
				continue;
			}

			if (UnitTableRowBase->MassEntityConfigAsset)
			{
				UnitTableRowBase->MassEntityConfigAsset->GetOrCreateEntityTemplate(*GetWorld());
			}

			CachedUnitTableRows.Add(UnitTableRowBase->UnitID, *UnitTableRowBase);
		}
	}
}

void UTWUnitSubsystem::PostInitialize()
{
	Super::PostInitialize();

	const UWorld* World = GetWorld();
	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(World);
	if (!ReplicationSubsystem)
	{
		return;
	}

	ReplicationSubsystem->RegisterBubbleInfoClass(ATWMassClientBubbleInfo::StaticClass());
}

void UTWUnitSubsystem::Deinitialize()
{
	UnitContainers.Empty();
	CachedUnitTableRows.Empty();
#ifdef WITH_CLIENT_CODE
	GClientUnitIdCache.Empty();
	GClientOwnerSlotCache.Empty();
	GClientCurrentHPCache.Empty();
	GClientMaxHPCache.Empty();
	GClientStatusCache.Empty();
#endif
	Super::Deinitialize();
}

#ifdef WITH_SERVER_CODE

void UTWUnitSubsystem::AddPlayer(int32 PlayerSlot)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	ATWGameState* GameState = Cast<ATWGameState>(GetWorld()->GetGameState());
	check(GameState);

	UTWPlayerUnitContainer* NewContainer = NewObject<UTWPlayerUnitContainer>(this);
	if (!NewContainer)
	{
		return;
	}

	NewContainer->Init(PlayerSlot);
	UnitContainers.Add(PlayerSlot, NewContainer);
}

bool UTWUnitSubsystem::FindNearestOwnedEntity(
	const FVector& Location,
	FMassEntityHandle& OutEntityHandle,
	int32 OwnerPlayerSlot,
	float MaxDistance)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	float MinSquaredDistance = FMath::Square(MaxDistance);
	FMassEntityHandle NearestEntity;

	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		TArray<FMassEntityHandle> EntityHandles;
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
			const FTWUnitFragment* UnitFrag = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
			if (!TransformFrag || !UnitFrag)
			{
				continue;
			}

			if (UnitFrag->GetOwner() != OwnerPlayerSlot)
			{
				continue;
			}

			const FVector EntityPos = TransformFrag->GetTransform().GetLocation();
			const float DistSq = FVector::DistSquared(Location, EntityPos);

			if (DistSq < MinSquaredDistance)
			{
				MinSquaredDistance = DistSq;
				NearestEntity = Entity;
			}
		}
	}

	if (NearestEntity.IsValid())
	{
		OutEntityHandle = NearestEntity;
		return true;
	}

	return false;
}

bool UTWUnitSubsystem::FindNearestAnyEntity(
	const FVector& Location,
	FMassEntityHandle& OutEntityHandle,
	float MaxDistance)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	float MinSquaredDistance = FMath::Square(MaxDistance);
	FMassEntityHandle NearestEntity;

	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		TArray<FMassEntityHandle> EntityHandles;
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
			if (!TransformFrag)
			{
				continue;
			}

			const FVector EntityPos = TransformFrag->GetTransform().GetLocation();
			const float DistSq = FVector::DistSquared(Location, EntityPos);

			if (DistSq < MinSquaredDistance)
			{
				MinSquaredDistance = DistSq;
				NearestEntity = Entity;
			}
		}
	}

	if (NearestEntity.IsValid())
	{
		OutEntityHandle = NearestEntity;
		return true;
	}

	return false;
}

bool UTWUnitSubsystem::FindNearestEnemyEntity(
	const FVector& Location,
	FMassEntityHandle& OutEntityHandle,
	int32 OwnerPlayerSlot,
	float MaxDistance)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	float MinSquaredDistance = FMath::Square(MaxDistance);
	FMassEntityHandle NearestEntity;

	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		TArray<FMassEntityHandle> EntityHandles;
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
			const FTWUnitFragment* UnitFrag = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
			if (!TransformFrag || !UnitFrag)
			{
				continue;
			}

			if (UnitFrag->GetOwner() == OwnerPlayerSlot)
			{
				continue;
			}

			const FVector EntityPos = TransformFrag->GetTransform().GetLocation();
			const float DistSq = FVector::DistSquared(Location, EntityPos);

			if (DistSq < MinSquaredDistance)
			{
				MinSquaredDistance = DistSq;
				NearestEntity = Entity;
			}
		}
	}

	if (NearestEntity.IsValid())
	{
		OutEntityHandle = NearestEntity;
		return true;
	}

	return false;
}

bool UTWUnitSubsystem::FindNearestEnemyBuilding(
	const FVector& Location,
	ATWBaseBuilding*& OutBuilding,
	int32 OwnerPlayerSlot,
	float MaxDistance)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	OutBuilding = nullptr;

	float MinSquaredDistance = FMath::Square(MaxDistance);

	for (TActorIterator<ATWBaseBuilding> It(GetWorld()); It; ++It)
	{
		ATWBaseBuilding* Building = *It;
		if (!IsValid(Building))
		{
			continue;
		}

		if (Building->GetOwnerPlayerSlot() == OwnerPlayerSlot)
		{
			continue;
		}

		if (!Building->CanBeAttacked())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Location, Building->GetAttackTargetLocation());
		if (DistSq < MinSquaredDistance)
		{
			MinSquaredDistance = DistSq;
			OutBuilding = Building;
		}
	}

	return OutBuilding != nullptr;
}

bool UTWUnitSubsystem::FindNearestEnemyBuildingCandidate(
	const FVector& Location,
	int32 OwnerPlayerSlot,
	FTWAttackableBuildingCandidate& OutCandidate,
	float MaxDistance)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	OutCandidate = FTWAttackableBuildingCandidate();

	ATWBaseBuilding* FoundBuilding = nullptr;
	if (!FindNearestEnemyBuilding(Location, FoundBuilding, OwnerPlayerSlot, MaxDistance))
	{
		return false;
	}

	OutCandidate.Building = FoundBuilding;
	OutCandidate.TargetLocation = FoundBuilding->GetAttackTargetLocation();
	OutCandidate.DistSquared = FVector::DistSquared(Location, OutCandidate.TargetLocation);
	return true;
}

bool UTWUnitSubsystem::GetEntitiesInRectangle(
	const FVector& StartLocation,
	const FVector& EndLocation,
	TArray<FMassEntityHandle>& OutEntityHandles,
	int32 OwnerPlayerSlot)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	OutEntityHandles.Empty();

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	const float MinX = FMath::Min(StartLocation.X, EndLocation.X);
	const float MaxX = FMath::Max(StartLocation.X, EndLocation.X);
	const float MinY = FMath::Min(StartLocation.Y, EndLocation.Y);
	const float MaxY = FMath::Max(StartLocation.Y, EndLocation.Y);

	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		TArray<FMassEntityHandle> EntityHandles;
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
			const FTWUnitFragment* UnitFrag = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
			if (!TransformFrag || !UnitFrag)
			{
				continue;
			}

			if (UnitFrag->GetOwner() != OwnerPlayerSlot)
			{
				continue;
			}

			const FVector EntityPos = TransformFrag->GetTransform().GetLocation();
			if (EntityPos.X >= MinX && EntityPos.X <= MaxX &&
				EntityPos.Y >= MinY && EntityPos.Y <= MaxY)
			{
				OutEntityHandles.Add(Entity);
			}
		}
	}

	return !OutEntityHandles.IsEmpty();
}
bool UTWUnitSubsystem::GetFriendlyEntitiesInRadius(
	const FVector& Center,
	float Radius,
	int32 OwnerPlayerSlot,
	TArray<FMassEntityHandle>& OutEntityHandles,
	bool bIncludeHeroes
)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	OutEntityHandles.Empty();

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	const float RadiusSq = FMath::Square(Radius);

	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		TArray<FMassEntityHandle> EntityHandles;
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
			const FTWUnitFragment* UnitFrag = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
			if (!TransformFrag || !UnitFrag)
			{
				continue;
			}

			if (UnitFrag->GetOwner() != OwnerPlayerSlot)
			{
				continue;
			}

			if (!bIncludeHeroes && IsHeroUnitId(UnitFrag->GetUnitID()))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(
				Center,
				TransformFrag->GetTransform().GetLocation()
			);

			if (DistSq <= RadiusSq)
			{
				OutEntityHandles.Add(Entity);
			}
		}
	}

	return OutEntityHandles.Num() > 0;
}

bool UTWUnitSubsystem::GetEnemyEntitiesInRadius(
	const FVector& Center,
	float Radius,
	int32 RequestOwnerPlayerSlot,
	TArray<FMassEntityHandle>& OutEntityHandles
)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	OutEntityHandles.Empty();

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return false;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	const float RadiusSq = FMath::Square(Radius);

	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		TArray<FMassEntityHandle> EntityHandles;
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
			const FTWUnitFragment* UnitFrag = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);
			if (!TransformFrag || !UnitFrag)
			{
				continue;
			}

			if (UnitFrag->GetOwner() == RequestOwnerPlayerSlot)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(
				Center,
				TransformFrag->GetTransform().GetLocation()
			);

			if (DistSq <= RadiusSq)
			{
				OutEntityHandles.Add(Entity);
			}
		}
	}

	return OutEntityHandles.Num() > 0;
}

bool UTWUnitSubsystem::FindNearestEnemyEntityAroundLocation(
	const FVector& Center,
	float SearchRadius,
	int32 RequestOwnerPlayerSlot,
	FMassEntityHandle& OutEntityHandle
)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	OutEntityHandle = FMassEntityHandle();

	TArray<FMassEntityHandle> Candidates;
	if (!GetEnemyEntitiesInRadius(Center, SearchRadius, RequestOwnerPlayerSlot, Candidates))
	{
		return false;
	}

	float BestDistSq = TNumericLimits<float>::Max();

	for (const FMassEntityHandle& Entity : Candidates)
	{
		FVector EntityLocation = FVector::ZeroVector;
		if (!TryGetUnitWorldLocation(Entity, EntityLocation))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Center, EntityLocation);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			OutEntityHandle = Entity;
		}
	}

	return OutEntityHandle.IsSet();
}

bool UTWUnitSubsystem::TryApplyDamageToEntity(
	const FMassEntityHandle& Entity,
	float DamageAmount
)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	if (DamageAmount <= 0.f)
	{
		return false;
	}

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager))
	{
		return false;
	}

	if (!IsSafeActiveEntity(EntityManager, Entity))
	{
		return false;
	}

	FTWStatusFragment* StatusFragment = EntityManager->GetFragmentDataPtr<FTWStatusFragment>(Entity);
	if (!StatusFragment)
	{
		return false;
	}

	FTWUnitStatus NewStatus = StatusFragment->GetStatus();
	const float CurrentHP = NewStatus.GetStatus(ETWStatusType::Health);
	if (CurrentHP <= 0.f)
	{
		return false;
	}

	NewStatus.SetStatus(
		ETWStatusType::Health,
		FMath::Max(0.f, CurrentHP - DamageAmount)
	);
	StatusFragment->SetStatus(NewStatus);

	return true;
}

void UTWUnitSubsystem::ApplyTemporaryMultiplierBuffToFriendlyUnits(
	const FVector& Center,
	float Radius,
	int32 OwnerPlayerSlot,
	float Multiplier,
	float Duration,
	const TArray<ETWStatusType>& TargetStatusTypes,
	bool bIncludeHeroes
)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	if (Multiplier <= 1.0f || Duration <= 0.0f || TargetStatusTypes.IsEmpty())
	{
		return;
	}

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassEntityHandle> TargetEntities;
	if (!GetFriendlyEntitiesInRadius(Center, Radius, OwnerPlayerSlot, TargetEntities, bIncludeHeroes))
	{
		return;
	}

	TArray<FTWTemporaryBuffEntry> AppliedEntries;
	AppliedEntries.Reserve(TargetEntities.Num());

	for (const FMassEntityHandle& Entity : TargetEntities)
	{
		FTWStatusFragment* StatusFragment = EntityManager.GetFragmentDataPtr<FTWStatusFragment>(Entity);
		if (!StatusFragment)
		{
			continue;
		}

		FTWUnitStatus DeltaStatus;
		bool bHasAnyDelta = false;

		for (const ETWStatusType StatusType : TargetStatusTypes)
		{
			// 현재 체력은 건드리지 않는 쪽이 안전
			if (StatusType == ETWStatusType::Health)
			{
				continue;
			}

			const float CurrentValue = StatusFragment->GetStatus().GetStatus(StatusType);
			const float DeltaValue = CurrentValue * (Multiplier - 1.0f);

			if (FMath::IsNearlyZero(DeltaValue))
			{
				continue;
			}

			DeltaStatus.SetStatus(StatusType, DeltaValue);
			bHasAnyDelta = true;
		}

		if (!bHasAnyDelta)
		{
			continue;
		}

		FTWUnitStatus NewStatus = StatusFragment->GetStatus();
		AddStatusDelta(NewStatus, DeltaStatus);
		StatusFragment->SetStatus(NewStatus);

		FTWTemporaryBuffEntry& NewEntry = AppliedEntries.AddDefaulted_GetRef();
		NewEntry.Entity = Entity;
		NewEntry.AppliedDelta = DeltaStatus;
	}

	if (AppliedEntries.IsEmpty())
	{
		return;
	}

	TWeakObjectPtr<UTWUnitSubsystem> WeakThis(this);

	FTimerHandle TempBuffTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TempBuffTimerHandle,
		[WeakThis, AppliedEntries]()
		{
			if (!WeakThis.IsValid() || !WeakThis->GetWorld())
			{
				return;
			}

			FMassEntityManager* LocalEntityManager = nullptr;
			if (!TryResolveActiveEntityManager(WeakThis->GetWorld(), LocalEntityManager))
			{
				return;
			}

			for (const FTWTemporaryBuffEntry& Entry : AppliedEntries)
			{
				if (!IsSafeActiveEntity(LocalEntityManager, Entry.Entity))
				{
					continue;
				}

				FTWStatusFragment* StatusFragment =
					LocalEntityManager->GetFragmentDataPtr<FTWStatusFragment>(Entry.Entity);
				if (!StatusFragment)
				{
					continue;
				}

				FTWUnitStatus NewStatus = StatusFragment->GetStatus();
				SubtractStatusDelta(NewStatus, Entry.AppliedDelta);
				StatusFragment->SetStatus(NewStatus);
			}
		},
		Duration,
		false
	);
}
void UTWUnitSubsystem::SpawnUnit(
	const FVector& Location,
	const FTWUnitTableRowBase& UnitTableRowBase,
	ATWPlayerController* PlayerController)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	UWorld* World = GetWorld();
	if (!IsValid(World) || !UnitTableRowBase.MassEntityConfigAsset)
	{
		return;
	}

	UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!MassSubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
	const FMassEntityTemplate& EntityTemplate =
		UnitTableRowBase.MassEntityConfigAsset->GetOrCreateEntityTemplate(*GetWorld());

	TWeakObjectPtr<ATWPlayerController> WeakPlayerController = PlayerController;
	TWeakObjectPtr<UTWUnitSubsystem> WeakThis = this;

	EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
		[Location, EntityTemplate, WeakPlayerController, WeakThis, UnitTableRowBase](
		FMassEntityManager& InOutEntityManager)
		{
			if (!WeakPlayerController.IsValid() || !WeakThis.IsValid())
			{
				return;
			}

			UWorld* LocalWorld = InOutEntityManager.GetWorld();
			if (!LocalWorld)
			{
				return;
			}

			UMassSpawnerSubsystem* MassSpawnerSubsystem = LocalWorld->GetSubsystem<UMassSpawnerSubsystem>();
			if (!MassSpawnerSubsystem)
			{
				return;
			}

			ATWPlayerState* PlayerState = WeakPlayerController->GetPlayerState<ATWPlayerState>();
			if (!IsValid(PlayerState))
			{
				return;
			}

			TArray<FMassEntityHandle> SpawnedEntities;
			MassSpawnerSubsystem->SpawnEntities(EntityTemplate, 1, SpawnedEntities);

			if (SpawnedEntities.Num() <= 0)
			{
				return;
			}

			FMassEntityHandle SpawnedUnit = SpawnedEntities[0];

			if (FTWUnitFragment* UnitFragment = InOutEntityManager.GetFragmentDataPtr<FTWUnitFragment>(SpawnedUnit))
			{
				UnitFragment->SetUnitID(UnitTableRowBase.UnitID);
				UnitFragment->SetOwner(PlayerState->PlayerSlot);
			}

			if (FTransformFragment* TransformFragment = InOutEntityManager.GetFragmentDataPtr<FTransformFragment>(SpawnedUnit))
			{
				TransformFragment->GetMutableTransform().SetLocation(Location);
			}

			if (FMassMoveTargetFragment* MoveTargetFragment =
				InOutEntityManager.GetFragmentDataPtr<FMassMoveTargetFragment>(SpawnedUnit))
			{
				MoveTargetFragment->Center = Location;
			}

			if (FTWStatusFragment* StatusFragment = InOutEntityManager.GetFragmentDataPtr<FTWStatusFragment>(SpawnedUnit))
			{
				const FTWUnitStatus UnitStatus =
					WeakThis->GetUnitDefaultStatus(UnitTableRowBase.UnitID, PlayerState->PlayerSlot);
				StatusFragment->SetStatus(UnitStatus);
			}

			WeakThis->AddUnit(PlayerState->PlayerSlot, SpawnedUnit);
		});
}

void UTWUnitSubsystem::OnUnitKilled(FMassEntityHandle& Unit)
{
	if (!GetWorld()->GetAuthGameMode())
	{
		return;
	}

	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(GetWorld());
	if (!EntityManager)
	{
		return;
	}

	FTWUnitFragment* UnitFragment = EntityManager->GetFragmentDataPtr<FTWUnitFragment>(Unit);
	if (!UnitFragment)
	{
		return;
	}

	const FName DeadUnitId = UnitFragment->GetUnitID();
	const int32 OwnerPlayerSlot = UnitFragment->GetOwner();

	if (IsHeroUnitId(DeadUnitId))
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!PC)
			{
				continue;
			}

			ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
			if (!PS)
			{
				continue;
			}

			if (PS->PlayerSlot == OwnerPlayerSlot)
			{
				if (ATWGameMode* GM = GetWorld()->GetAuthGameMode<ATWGameMode>())
				{
					GM->RequestRespawnHero(PS, DeadUnitId, 5.0f);
				}
				break;
			}
		}
	}

	RemoveUnit(OwnerPlayerSlot, UnitFragment->GetIdx());
}

TMap<EResourceType, int32> UTWUnitSubsystem::GetUpkeep(int32 PlayerSlot)
{
	if (!UnitContainers.Contains(PlayerSlot) || !UnitContainers[PlayerSlot])
	{
		return {};
	}

	return UnitContainers[PlayerSlot]->GetUpkeep();
}

int32 UTWUnitSubsystem::GetCurrentPopulation(int32 PlayerSlot) const
{
	if (!UnitContainers.Contains(PlayerSlot) || !UnitContainers[PlayerSlot])
	{
		return 0;
	}

	return UnitContainers[PlayerSlot]->GetCurrentPopulation();
}

void UTWUnitSubsystem::AddUnit(int32 PlayerSlot, FMassEntityHandle& Unit)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	if (!Unit.IsValid())
	{
		return;
	}

	if (!UnitContainers.Contains(PlayerSlot) || !UnitContainers[PlayerSlot])
	{
		return;
	}

	UnitContainers[PlayerSlot]->AddUnit(Unit);
}

void UTWUnitSubsystem::RemoveUnit(int32 PlayerSlot, int32 Idx)
{
	if (!GetWorld()->GetAuthGameMode())
	{
		return;
	}

	if (!UnitContainers.Contains(PlayerSlot) || !UnitContainers[PlayerSlot])
	{
		return;
	}

	UnitContainers[PlayerSlot]->RemoveUnit(Idx);
}

void UTWUnitSubsystem::ApplyStatus(FName UnitID, int32 PlayerSlot)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	if (!UnitContainers.Contains(PlayerSlot) || !UnitContainers[PlayerSlot])
	{
		return;
	}

	const FTWUnitStatus UnitStatus = GetUnitDefaultStatus(UnitID, PlayerSlot);
	UnitContainers[PlayerSlot]->ApplyStatus(UnitID, UnitStatus);
}
bool UTWUnitSubsystem::TryGetUnitWorldLocation(
	const FMassEntityHandle& Entity,
	FVector& OutLocation
) const
{
	OutLocation = FVector::ZeroVector;

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager))
	{
		return false;
	}

	if (!IsSafeActiveEntity(EntityManager, Entity))
	{
		return false;
	}

	const FTransformFragment* TransformFragment =
		EntityManager->GetFragmentDataPtr<FTransformFragment>(Entity);

	if (!TransformFragment)
	{
		return false;
	}

	OutLocation = TransformFragment->GetTransform().GetLocation();
	return true;
}

bool UTWUnitSubsystem::SpawnUnitById(
	const FVector& Location,
	FName UnitId,
	ATWPlayerController* PlayerController
)
{
	if (!GetWorld() || !GetWorld()->GetAuthGameMode())
	{
		return false;
	}

	const FTWUnitTableRowBase* UnitRow = GetUnitTableRowBase(UnitId);
	if (!UnitRow)
	{
		UE_LOG(LogTWSubsystem, Warning, TEXT("[UnitSubsystem] SpawnUnitById failed: UnitRow not found (%s)"), *UnitId.ToString());
		return false;
	}

	SpawnUnit(Location, *UnitRow, PlayerController);
	return true;
}

bool UTWUnitSubsystem::SpawnHeroUnitById(
	const FVector& Location,
	FName HeroUnitId,
	ATWPlayerController* PlayerController
)
{
	if (!GetWorld() || !GetWorld()->GetAuthGameMode())
	{
		return false;
	}

	if (!IsHeroUnitId(HeroUnitId))
	{
		UE_LOG(LogTWSubsystem, Warning, TEXT("[UnitSubsystem] SpawnHeroUnitById failed: Not hero id (%s)"), *HeroUnitId.ToString());
		return false;
	}

	return SpawnUnitById(Location, HeroUnitId, PlayerController);
}

bool UTWUnitSubsystem::IsHeroUnitId(FName UnitId) const
{
	return
		UnitId == TEXT("DragonKnight") ||
		UnitId == TEXT("Markman") ||
		UnitId == TEXT("Astrologian");
}

#endif

FTWUnitStatus UTWUnitSubsystem::GetUnitDefaultStatus(FName UnitID, int32 PlayerSlot)
{
	FTWUnitStatus UnitStatus;

	if (const FTWUnitTableRowBase* UnitRow = GetUnitTableRowBase(UnitID))
	{
		UnitStatus = UnitRow->BaseStatus;
	}

	ATWGameState* GameState = Cast<ATWGameState>(GetWorld()->GetGameState());
	if (!IsValid(GameState))
	{
		return UnitStatus;
	}

	ATWPlayerState* PlayerState = GameState->GetPlayerState(PlayerSlot);
	if (!IsValid(PlayerState))
	{
		return UnitStatus;
	}

	UnitStatus += PlayerState->GetUnitUpgradeBonus(UnitID);
	return UnitStatus;
}

FTWUnitStatus UTWUnitSubsystem::GetUnitCurrentStatus(const FMassEntityHandle& Unit, int32 PlayerSlot) const
{
	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager))
	{
		return FTWUnitStatus();
	}

	if (!IsSafeActiveEntity(EntityManager, Unit))
	{
		return FTWUnitStatus();
	}

	const FTWStatusFragment* StatusFragment = EntityManager->GetFragmentDataPtr<FTWStatusFragment>(Unit);
	if (!StatusFragment)
	{
		return FTWUnitStatus();
	}

	return StatusFragment->GetStatus();
}

const FTWUnitTableRowBase* UTWUnitSubsystem::GetUnitTableRowBase(FName UnitID) const
{
	return CachedUnitTableRows.Find(UnitID);
}

#ifdef WITH_CLIENT_CODE

bool UTWUnitSubsystem::TryGetReplicationEntityInfo(
	const FMassNetworkID& UnitNetID,
	const FMassReplicationEntityInfo*& OutEntityInfo) const
{
	OutEntityInfo = nullptr;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UMassReplicationSubsystem* RepSubsystem = World->GetSubsystem<UMassReplicationSubsystem>();
	if (!IsValid(RepSubsystem))
	{
		return false;
	}

	const FMassReplicationEntityInfo* EntityInfo = RepSubsystem->GetEntityInfoMap().Find(UnitNetID);
	if (!EntityInfo || !EntityInfo->Entity.IsValid())
	{
		return false;
	}

	OutEntityInfo = EntityInfo;
	return true;
}

FTWUnitStatus UTWUnitSubsystem::GetUnitCurrentStatus(const FMassNetworkID& UnitNetID, int32 PlayerSlot) const
{
	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
		{
			return *CachedStatus;
		}
		return FTWUnitStatus();
	}

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager))
	{
		if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
		{
			return *CachedStatus;
		}
		return FTWUnitStatus();
	}

	if (!IsSafeActiveEntity(EntityManager, EntityInfo->Entity))
	{
		if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
		{
			return *CachedStatus;
		}
		return FTWUnitStatus();
	}

	const FTWUnitStatus Status = GetUnitCurrentStatus(EntityInfo->Entity, PlayerSlot);
	GClientStatusCache.Add(UnitNetID, Status);
	return Status;
}

bool UTWUnitSubsystem::TryGetUnitID(const FMassNetworkID& UnitNetID, FName& OutUnitID) const
{
	OutUnitID = NAME_None;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		if (const FName* CachedUnitId = GClientUnitIdCache.Find(UnitNetID))
		{
			OutUnitID = *CachedUnitId;
			return !OutUnitID.IsNone();
		}
		return false;
	}

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager) || !IsSafeActiveEntity(EntityManager, EntityInfo->Entity))
	{
		if (const FName* CachedUnitId = GClientUnitIdCache.Find(UnitNetID))
		{
			OutUnitID = *CachedUnitId;
			return !OutUnitID.IsNone();
		}
		return false;
	}

	const FTWUnitFragment* UnitFragment = EntityManager->GetFragmentDataPtr<FTWUnitFragment>(EntityInfo->Entity);
	if (!UnitFragment)
	{
		if (const FName* CachedUnitId = GClientUnitIdCache.Find(UnitNetID))
		{
			OutUnitID = *CachedUnitId;
			return !OutUnitID.IsNone();
		}
		return false;
	}

	OutUnitID = UnitFragment->GetUnitID();
	if (!OutUnitID.IsNone())
	{
		GClientUnitIdCache.Add(UnitNetID, OutUnitID);
	}
	return !OutUnitID.IsNone();
}

bool UTWUnitSubsystem::TryGetUnitOwnerPlayerSlot(
	const FMassNetworkID& UnitNetID,
	int32& OutOwnerPlayerSlot) const
{
	OutOwnerPlayerSlot = INDEX_NONE;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		if (const int32* CachedOwner = GClientOwnerSlotCache.Find(UnitNetID))
		{
			OutOwnerPlayerSlot = *CachedOwner;
			return true;
		}
		return false;
	}

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager) || !IsSafeActiveEntity(EntityManager, EntityInfo->Entity))
	{
		if (const int32* CachedOwner = GClientOwnerSlotCache.Find(UnitNetID))
		{
			OutOwnerPlayerSlot = *CachedOwner;
			return true;
		}
		return false;
	}

	const FTWUnitFragment* UnitFragment = EntityManager->GetFragmentDataPtr<FTWUnitFragment>(EntityInfo->Entity);
	if (!UnitFragment)
	{
		if (const int32* CachedOwner = GClientOwnerSlotCache.Find(UnitNetID))
		{
			OutOwnerPlayerSlot = *CachedOwner;
			return true;
		}
		return false;
	}

	OutOwnerPlayerSlot = UnitFragment->GetOwner();
	GClientOwnerSlotCache.Add(UnitNetID, OutOwnerPlayerSlot);
	return OutOwnerPlayerSlot != INDEX_NONE;
}

bool UTWUnitSubsystem::TryGetUnitVisualActor(
	const FMassNetworkID& UnitNetID,
	const ATWUnit*& OutVisualActor) const
{
	OutVisualActor = nullptr;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		return false;
	}

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager))
	{
		return false;
	}

	if (!IsSafeActiveEntity(EntityManager, EntityInfo->Entity))
	{
		return false;
	}

	const FMassActorFragment* ActorFragment =
		EntityManager->GetFragmentDataPtr<FMassActorFragment>(EntityInfo->Entity);

	if (!ActorFragment || !ActorFragment->IsValid())
	{
		return false;
	}

	const AActor* Actor = ActorFragment->Get();
	if (!IsValid(Actor))
	{
		return false;
	}

	const ATWUnit* TWUnit = Cast<ATWUnit>(Actor);
	if (!IsValid(TWUnit))
	{
		return false;
	}

	OutVisualActor = TWUnit;
	return true;
}

bool UTWUnitSubsystem::TryGetUnitLocationInternal(
	const FMassNetworkID& UnitNetID,
	FVector& OutLocation) const
{
	OutLocation = FVector::ZeroVector;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		return false;
	}

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager))
	{
		return false;
	}

	if (!IsSafeActiveEntity(EntityManager, EntityInfo->Entity))
	{
		return false;
	}

	const FTransformFragment* TransformFragment =
		EntityManager->GetFragmentDataPtr<FTransformFragment>(EntityInfo->Entity);

	if (!TransformFragment)
	{
		return false;
	}

	OutLocation = TransformFragment->GetTransform().GetLocation();
	return true;
}

bool UTWUnitSubsystem::TryProjectWorldPointToGround(
	const FVector& InWorldPoint,
	FVector& OutGroundPoint,
	const AActor* ActorToIgnore) const
{
	OutGroundPoint = InWorldPoint;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector TraceStart = InWorldPoint + FVector(0.f, 0.f, 300.f);
	const FVector TraceEnd = InWorldPoint - FVector(0.f, 0.f, 800.f);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TWSelectionGroundTrace), false);
	Params.bTraceComplex = false;

	if (IsValid(ActorToIgnore))
	{
		Params.AddIgnoredActor(ActorToIgnore);
	}

	if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return false;
	}

	OutGroundPoint = Hit.Location;
	return true;
}

bool UTWUnitSubsystem::TryGetUnitVisualLocation(const FMassNetworkID& UnitNetID, FVector& OutLocation) const
{
	OutLocation = FVector::ZeroVector;

	const ATWUnit* TWUnit = nullptr;
	if (TryGetUnitVisualActor(UnitNetID, TWUnit) && IsValid(TWUnit))
	{
		const FVector AnchorWorldLocation = TWUnit->GetSelectionAnchorWorldLocation();

		FVector GroundLocation = FVector::ZeroVector;
		if (TryProjectWorldPointToGround(AnchorWorldLocation, GroundLocation, TWUnit))
		{
			OutLocation = GroundLocation;
			CachedClientVisualLocations.Add(UnitNetID, OutLocation);
			return true;
		}

		OutLocation = AnchorWorldLocation;
		CachedClientVisualLocations.Add(UnitNetID, OutLocation);
		return true;
	}

	FVector BaseLocation = FVector::ZeroVector;
	if (!TryGetUnitLocationInternal(UnitNetID, BaseLocation))
	{
		if (const FVector* CachedLocation = CachedClientVisualLocations.Find(UnitNetID))
		{
			OutLocation = *CachedLocation;
			return true;
		}
		return false;
	}

	FVector GroundLocation = FVector::ZeroVector;
	if (TryProjectWorldPointToGround(BaseLocation, GroundLocation, nullptr))
	{
		OutLocation = GroundLocation;
		CachedClientVisualLocations.Add(UnitNetID, OutLocation);
		return true;
	}

	OutLocation = BaseLocation;
	CachedClientVisualLocations.Add(UnitNetID, OutLocation);
	return true;
}

bool UTWUnitSubsystem::TryGetUnitHPBarWorldLocation(const FMassNetworkID& UnitNetID, FVector& OutLocation) const
{
	OutLocation = FVector::ZeroVector;

	const ATWUnit* TWUnit = nullptr;
	if (TryGetUnitVisualActor(UnitNetID, TWUnit) && IsValid(TWUnit))
	{
		OutLocation = TWUnit->GetHPBarAnchorWorldLocation();
		CachedClientHPBarLocations.Add(UnitNetID, OutLocation);
		return true;
	}

	FVector BaseLocation = FVector::ZeroVector;
	if (!TryGetUnitLocationInternal(UnitNetID, BaseLocation))
	{
		if (const FVector* CachedLocation = CachedClientHPBarLocations.Find(UnitNetID))
		{
			OutLocation = *CachedLocation;
			return true;
		}
		return false;
	}

	OutLocation = BaseLocation + FVector(0.f, 0.f, 120.f);
	CachedClientHPBarLocations.Add(UnitNetID, OutLocation);
	return true;
}

bool UTWUnitSubsystem::TryGetUnitCurrentHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutCurrentHP) const
{
	OutCurrentHP = 0.f;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		if (const float* CachedHP = GClientCurrentHPCache.Find(UnitNetID))
		{
			OutCurrentHP = *CachedHP;
			return true;
		}
		if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
		{
			OutCurrentHP = CachedStatus->GetStatus(ETWStatusType::Health);
			return OutCurrentHP > 0.f;
		}
		return false;
	}

	FMassEntityManager* EntityManager = nullptr;
	if (!TryResolveActiveEntityManager(GetWorld(), EntityManager) || !IsSafeActiveEntity(EntityManager, EntityInfo->Entity))
	{
		if (const float* CachedHP = GClientCurrentHPCache.Find(UnitNetID))
		{
			OutCurrentHP = *CachedHP;
			return true;
		}
		if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
		{
			OutCurrentHP = CachedStatus->GetStatus(ETWStatusType::Health);
			return OutCurrentHP > 0.f;
		}
		return false;
	}

	const FTWStatusFragment* StatusFragment = EntityManager->GetFragmentDataPtr<FTWStatusFragment>(EntityInfo->Entity);
	if (!StatusFragment)
	{
		if (const float* CachedHP = GClientCurrentHPCache.Find(UnitNetID))
		{
			OutCurrentHP = *CachedHP;
			return true;
		}
		if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
		{
			OutCurrentHP = CachedStatus->GetStatus(ETWStatusType::Health);
			return OutCurrentHP > 0.f;
		}
		return false;
	}

	const FTWUnitStatus Status = StatusFragment->GetStatus();
	GClientStatusCache.Add(UnitNetID, Status);
	OutCurrentHP = Status.GetStatus(ETWStatusType::Health);
	GClientCurrentHPCache.Add(UnitNetID, OutCurrentHP);
	return true;
}

bool UTWUnitSubsystem::TryGetUnitMaxHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutMaxHP) const
{
	OutMaxHP = 0.f;

	FName UnitID = NAME_None;
	if (!TryGetUnitID(UnitNetID, UnitID))
	{
		if (const float* CachedMax = GClientMaxHPCache.Find(UnitNetID))
		{
			OutMaxHP = *CachedMax;
			return OutMaxHP > 0.f;
		}
		return false;
	}

	int32 ResolvedOwnerPlayerSlot = PlayerSlot;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (TryGetReplicationEntityInfo(UnitNetID, EntityInfo) && EntityInfo)
	{
		FMassEntityManager* EntityManager = nullptr;
		if (TryResolveActiveEntityManager(GetWorld(), EntityManager) &&
			IsSafeActiveEntity(EntityManager, EntityInfo->Entity))
		{
			const FTWUnitFragment* UnitFragment =
				EntityManager->GetFragmentDataPtr<FTWUnitFragment>(EntityInfo->Entity);

			if (UnitFragment)
			{
				ResolvedOwnerPlayerSlot = UnitFragment->GetOwner();
			}
		}
	}

	const FTWUnitTableRowBase* UnitRow = GetUnitTableRowBase(UnitID);
	if (!UnitRow)
	{
		if (const float* CachedMax = GClientMaxHPCache.Find(UnitNetID))
		{
			OutMaxHP = *CachedMax;
			return OutMaxHP > 0.f;
		}
		if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
		{
			OutMaxHP = CachedStatus->GetStatus(ETWStatusType::Health);
			return OutMaxHP > 0.f;
		}
		return false;
	}

	const FTWUnitStatus DefaultStatus =
		const_cast<UTWUnitSubsystem*>(this)->GetUnitDefaultStatus(UnitID, ResolvedOwnerPlayerSlot);

	OutMaxHP = DefaultStatus.GetStatus(ETWStatusType::Health);
	if (OutMaxHP > 0.f)
	{
		GClientMaxHPCache.Add(UnitNetID, OutMaxHP);
		return true;
	}

	if (const FTWUnitStatus* CachedStatus = GClientStatusCache.Find(UnitNetID))
	{
		OutMaxHP = CachedStatus->GetStatus(ETWStatusType::Health);
		if (OutMaxHP > 0.f)
		{
			GClientMaxHPCache.Add(UnitNetID, OutMaxHP);
			return true;
		}
	}

	if (const float* CachedMax = GClientMaxHPCache.Find(UnitNetID))
	{
		OutMaxHP = *CachedMax;
		return OutMaxHP > 0.f;
	}

	return false;
}

bool UTWUnitSubsystem::TryGetUnitSelectionVisualStyle(
	const FMassNetworkID& UnitNetID,
	FTWUnitSelectionVisualStyle& OutStyle) const
{
	OutStyle = FTWUnitSelectionVisualStyle();

	FName UnitID = NAME_None;
	if (!TryGetUnitID(UnitNetID, UnitID))
	{
		return false;
	}

	return true;
}

#endif
