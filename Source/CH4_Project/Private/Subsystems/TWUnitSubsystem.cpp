#include "Subsystems/TWUnitSubsystem.h"

#include "Core/TWPlayerUnitContainer.h"
#include "Core/TWGameState.h"
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
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Replication/BubbleInfo/TWMassClientBubbleInfo.h"

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
		for (FTWUnitTableRowBase* UnitTableRowBase : UnitTableRowBases)
		{
			if (!UnitTableRowBase)
			{
				continue;
			}

			if (UnitTableRowBase->MassEntityConfigAsset)
			{
				UnitTableRowBase->MassEntityConfigAsset->GetOrCreateEntityTemplate(*GetWorld());
			}

			CachedUnitTableRows.Add(UnitTableRowBase->UnitID, UnitTableRowBase);
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

bool UTWUnitSubsystem::FindNearestEntity(
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

bool UTWUnitSubsystem::GetEntitiesInRectangle(
	const FVector& StartLocation,
	const FVector& EndLocation,
	TArray<FMassEntityHandle>& OutEntityHandles,
	int32 OwnerPlayerSlot)
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
			const double DotResult = FVector::DotProduct(StartLocation - EntityPos, EndLocation - EntityPos);
			if (DotResult < 0.0)
			{
				OutEntityHandles.Add(Entity);
			}
		}
	}

	return !OutEntityHandles.IsEmpty();
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
	TWeakObjectPtr<ThisClass> WeakThis = this;

	EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
		[Location, EntityTemplate, WeakPlayerController, WeakThis, UnitTableRowBase](FMassEntityManager& InOutEntityManager)
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

			if (FTransformFragment* TransformFragment =
				InOutEntityManager.GetFragmentDataPtr<FTransformFragment>(SpawnedUnit))
			{
				TransformFragment->GetMutableTransform().SetLocation(Location);
			}

			if (FMassMoveTargetFragment* MoveTargetFragment =
				InOutEntityManager.GetFragmentDataPtr<FMassMoveTargetFragment>(SpawnedUnit))
			{
				MoveTargetFragment->Center = Location;
			}

			if (FTWStatusFragment* StatusFragment =
				InOutEntityManager.GetFragmentDataPtr<FTWStatusFragment>(SpawnedUnit))
			{
				const FTWUnitStatus UnitStatus =
					WeakThis->GetUnitDefaultStatus(UnitTableRowBase.UnitID, PlayerState->PlayerSlot);
				StatusFragment->SetStatus(UnitStatus);
			}

			WeakThis->AddUnit(PlayerState->PlayerSlot, SpawnedUnit);
		});
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
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

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

#endif

FTWUnitStatus UTWUnitSubsystem::GetUnitDefaultStatus(FName UnitID, int32 PlayerSlot)
{
	FTWUnitStatus UnitStatus;

	if (FTWUnitTableRowBase* UnitRow = GetUnitTableRowBase(UnitID))
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

FTWUnitStatus UTWUnitSubsystem::GetUnitCurrentStatus(const FMassEntityHandle& Unit, int32 PlayerSlot)
{
	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(GetWorld());
	if (!EntityManager || !Unit.IsValid())
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

FTWUnitTableRowBase* UTWUnitSubsystem::GetUnitTableRowBase(FName UnitID) const
{
	if (FTWUnitTableRowBase* const* FoundRow = CachedUnitTableRows.Find(UnitID))
	{
		return *FoundRow;
	}

	return nullptr;
}

#ifdef WITH_CLIENT_CODE

bool UTWUnitSubsystem::TryGetReplicationEntityInfo(
	const FMassNetworkID& UnitNetID,
	const FMassReplicationEntityInfo*& OutEntityInfo
) const
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

FTWUnitStatus UTWUnitSubsystem::GetUnitCurrentStatus(const FMassNetworkID& UnitNetID, int32 PlayerSlot)
{
	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		return FTWUnitStatus();
	}

	return GetUnitCurrentStatus(EntityInfo->Entity, PlayerSlot);
}

bool UTWUnitSubsystem::TryGetUnitID(const FMassNetworkID& UnitNetID, FName& OutUnitID) const
{
	OutUnitID = NAME_None;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		return false;
	}

	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(GetWorld());
	if (!EntityManager)
	{
		return false;
	}

	const FTWUnitFragment* UnitFragment =
		EntityManager->GetFragmentDataPtr<FTWUnitFragment>(EntityInfo->Entity);
	if (!UnitFragment)
	{
		return false;
	}

	OutUnitID = UnitFragment->GetUnitID();
	return OutUnitID != NAME_None;
}

bool UTWUnitSubsystem::TryGetUnitTWUnit(
	const FMassNetworkID& UnitNetID,
	const ATWUnit*& OutTWUnit
) const
{
	OutTWUnit = nullptr;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		return false;
	}

	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(GetWorld());
	if (!EntityManager)
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

	OutTWUnit = TWUnit;
	return true;
}

bool UTWUnitSubsystem::TryGetUnitLocationInternal(
	const FMassNetworkID& UnitNetID,
	FVector& OutLocation
) const
{
	OutLocation = FVector::ZeroVector;

	const FMassReplicationEntityInfo* EntityInfo = nullptr;
	if (!TryGetReplicationEntityInfo(UnitNetID, EntityInfo) || !EntityInfo)
	{
		return false;
	}

	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(GetWorld());
	if (!EntityManager)
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
	const AActor* ActorToIgnore
) const
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
	if (TryGetUnitTWUnit(UnitNetID, TWUnit) && IsValid(TWUnit))
	{
		const FVector AnchorWorldLocation = TWUnit->GetSelectionAnchorWorldLocation();

		FVector GroundLocation = FVector::ZeroVector;
		if (TryProjectWorldPointToGround(AnchorWorldLocation, GroundLocation, TWUnit))
		{
			OutLocation = GroundLocation;
			return true;
		}

		OutLocation = AnchorWorldLocation;
		return true;
	}

	FVector BaseLocation = FVector::ZeroVector;
	if (!TryGetUnitLocationInternal(UnitNetID, BaseLocation))
	{
		return false;
	}

	FVector GroundLocation = FVector::ZeroVector;
	if (TryProjectWorldPointToGround(BaseLocation, GroundLocation, nullptr))
	{
		OutLocation = GroundLocation;
		return true;
	}

	OutLocation = BaseLocation;
	return true;
}

bool UTWUnitSubsystem::TryGetUnitHPBarWorldLocation(const FMassNetworkID& UnitNetID, FVector& OutLocation) const
{
	OutLocation = FVector::ZeroVector;

	const ATWUnit* TWUnit = nullptr;
	if (TryGetUnitTWUnit(UnitNetID, TWUnit) && IsValid(TWUnit))
	{
		OutLocation = TWUnit->GetHPBarAnchorWorldLocation();
		return true;
	}

	FVector BaseLocation = FVector::ZeroVector;
	if (!TryGetUnitLocationInternal(UnitNetID, BaseLocation))
	{
		return false;
	}

	OutLocation = BaseLocation + FVector(0.f, 0.f, 120.f);
	return true;
}

bool UTWUnitSubsystem::TryGetUnitCurrentHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutCurrentHP) const
{
	OutCurrentHP = 0.f;

	const FTWUnitStatus Status =
		const_cast<UTWUnitSubsystem*>(this)->GetUnitCurrentStatus(UnitNetID, PlayerSlot);

	OutCurrentHP = Status.GetStatus(ETWStatusType::Health);
	return OutCurrentHP > 0.f;
}

bool UTWUnitSubsystem::TryGetUnitMaxHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutMaxHP) const
{
	OutMaxHP = 0.f;

	FName UnitID = NAME_None;
	if (!TryGetUnitID(UnitNetID, UnitID))
	{
		return false;
	}

	if (FTWUnitTableRowBase* UnitRow = const_cast<UTWUnitSubsystem*>(this)->GetUnitTableRowBase(UnitID))
	{
		OutMaxHP = UnitRow->BaseStatus.GetStatus(ETWStatusType::Health);
		return OutMaxHP > 0.f;
	}

	return false;
}

bool UTWUnitSubsystem::TryGetUnitSelectionVisualStyle(
	const FMassNetworkID& UnitNetID,
	FTWUnitSelectionVisualStyle& OutStyle
) const
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