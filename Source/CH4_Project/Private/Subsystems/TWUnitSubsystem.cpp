// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystems/TWUnitSubsystem.h"
#include "Core/TWPlayerUnitContainer.h"
#include "EngineUtils.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassNavigationSubsystem.h"
#include "MassReplicationSubsystem.h"
#include "MassEntityConfigAsset.h"
#include "MassNavigationFragments.h"
#include "MassSpawner.h"
#include "Core/TWGameState.h"
#include "Core/TWPlayerState.h"
#include "Core/TWPlayerController.h" 
#include "Data/TWUnitTableRowBase.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Replication/BubbleInfo/TWStatusMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleInfo/TWTransformMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleInfo/TWTransformSmoothMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleInfo/TWUnitMassClientBubbleInfo.h"

UTWUnitSubsystem::UTWUnitSubsystem()
{
}

void UTWUnitSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	FString Path = TEXT("/Game/CH4_Project/Datas/Tables/DT_UnitTableRowBase.DT_UnitTableRowBase");
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

	ReplicationSubsystem->RegisterBubbleInfoClass(ATWTransformMassClientBubbleInfo::StaticClass());
	ReplicationSubsystem->RegisterBubbleInfoClass(ATWTransformSmoothMassClientBubbleInfo::StaticClass());
	ReplicationSubsystem->RegisterBubbleInfoClass(ATWStatusMassClientBubbleInfo::StaticClass());
	ReplicationSubsystem->RegisterBubbleInfoClass(ATWUnitMassClientBubbleInfo::StaticClass());
}

void UTWUnitSubsystem::Deinitialize()
{
	UnitContainers.Empty();
	CachedUnitTableRows.Empty();
	Super::Deinitialize();
}

void UTWUnitSubsystem::AddPlayer(int32 PlayerSlot)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	ATWGameState* GameState = Cast<ATWGameState>(GetWorld()->GetGameState());
	check(GameState);

	UnitContainers.Add(PlayerSlot, NewObject<UTWPlayerUnitContainer>(this));
	UnitContainers[PlayerSlot]->Init(PlayerSlot);
}

#ifdef WITH_SERVER_CODE

// TODO FMassEntityHandle을 이 시스템에서 별도로 관리하고 Spatial Hasing적용해서 순회해야함
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

			// 내 유닛만 선택 대상으로 제한
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
			if (!WeakPlayerController.IsValid())
			{
				return;
			}

			if (!WeakThis.IsValid())
			{
				return;
			}

			UWorld* World = InOutEntityManager.GetWorld();
			if (!World)
			{
				return;
			}

			UMassSpawnerSubsystem* MassSpawnerSubsystem = World->GetSubsystem<UMassSpawnerSubsystem>();
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
			
			if (FMassNetworkIDFragment* NetworkIDFragment = InOutEntityManager.GetFragmentDataPtr<FMassNetworkIDFragment>(SpawnedUnit))
			{
				FMassNetworkID NetID = NetworkIDFragment->NetID;
				UE_LOG(LogTemp, Warning, TEXT("%d"), NetID.GetValue());
			}
			
			
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

			// TODO Stat HardCoded
			// TODO 유닛 아이디 fragment에 주입
			// TODO 업그레이드 현황에 따른 스탯 설정
			if (FTWStatusFragment* StatusFragment =
				InOutEntityManager.GetFragmentDataPtr<FTWStatusFragment>(SpawnedUnit))
			{
				StatusFragment->SetStatus(UnitTableRowBase.BaseStatus);
			}

			WeakThis->AddUnit(PlayerState->PlayerSlot, SpawnedUnit);

			// PendingPopulation 차감은 호출부(생산 완료 처리)에서만 하도록 유지
			// PlayerState->RemovePendingPopulation(UnitTableRowBase.Population);
		});
}

TMap<EResourceType, int32> UTWUnitSubsystem::GetUpkeep(int32 PlayerSlot)
{
	return UnitContainers[PlayerSlot]->GetUpkeep();
}

int32 UTWUnitSubsystem::GetCurrentPopulation(int32 PlayerSlot) const
{
	return UnitContainers[PlayerSlot]->GetCurrentPopulation();
}

void UTWUnitSubsystem::AddUnit(int32 PlayerSlot, FMassEntityHandle& Unit)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));
	
	if (!Unit.IsValid())
	{
		return;
	}

	// [MOD] 컨테이너 누락 방어
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

FTWUnitTableRowBase* UTWUnitSubsystem::GetUnitTableRowBase(FName UnitID) const
{
	if (FTWUnitTableRowBase* const* FoundRow = CachedUnitTableRows.Find(UnitID))
	{
		return *FoundRow;
	}

	return nullptr;
}
#endif