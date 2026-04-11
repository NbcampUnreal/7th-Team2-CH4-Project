// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/TWUnitSubsystem.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassNavigationSubsystem.h"
#include "MassReplicationSubsystem.h"
#include "MassEntityConfigAsset.h"
#include "MassSpawner.h"
#include "Core/TWGameState.h"
#include "Core/TWPlayerState.h"
#include "Data/TWUnitTableRowBase.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Replication/BubbleInfo/TWTransformMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleInfo/TWTransformSmoothMassClientBubbleInfo.h"

// UTWUnitSubsystem::UTWUnitSubsystem()
// {
// 	// ConstructorHelpers::FObjectFinder<UDataTable> f(TEXT("/Game/CH4_Project/Mass/MASSTEST/NewDataTable.NewDataTable"));
// 	// if (f.Succeeded())
// 	// {
// 	// 	UnitTable = f.Object;
// 	// }
// }

void UTWUnitSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (IsValid(EntitySubsystem))
	{
		FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
		FindNearestEntityQuery.Initialize(EntityManager.AsShared());
		FindNearestEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	}
	TArray<FTWUnitTableRowBase*> UnitTableRowBases;
	if (UnitTable)
	{
		UnitTable->GetAllRows<FTWUnitTableRowBase>(TEXT("UnitSubsystem::OnWorldBeginPlay"), UnitTableRowBases);
		for (FTWUnitTableRowBase* UnitTableRowBase : UnitTableRowBases)
		{
			CachedUnitTableRows.Add(UnitTableRowBase->UnitID, UnitTableRowBase);
		}
	}
}

void UTWUnitSubsystem::PostInitialize()
{
	Super::PostInitialize();
	const UWorld* World = GetWorld();
	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(World);
	ReplicationSubsystem->RegisterBubbleInfoClass(ATWTransformMassClientBubbleInfo::StaticClass());
	ReplicationSubsystem->RegisterBubbleInfoClass(ATWTransformSmoothMassClientBubbleInfo::StaticClass());
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
	UnitContainers[PlayerSlot] = NewObject<UTWPlayerUnitContainer>();
	UnitContainers[PlayerSlot]->Init(PlayerSlot);
}

#ifdef WITH_SERVER_CODE

//TODO FMassEntityHandle을 이 시스템에서 별도로 관리하고 Spatial Hasing적용해서 순회해야함
//현재는 단순히 월드에 존재하는 모든 엔티티를 조회함.
bool UTWUnitSubsystem::FindNearestEntity(const FVector& Location, FMassEntityHandle& OutEntityHandle, float MaxDistance)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return false;

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	float MinSquaredDistance = FMath::Square(MaxDistance);
	FMassEntityHandle NearestEntity;

	TArray<FMassEntityHandle> EntityHandles;
	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			if (const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity))
			{
				const FVector EntityPos = TransformFrag->GetTransform().GetLocation();
				const float DistSq = FVector::DistSquared(Location, EntityPos);

				if (DistSq < MinSquaredDistance)
				{
					MinSquaredDistance = DistSq;
					NearestEntity = Entity;
				}
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

bool UTWUnitSubsystem::GetEntitiesInRectangle(const FVector& StartLocation, const FVector& EndLocation,
                                              TArray<FMassEntityHandle>& OutEntityHandles)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return false;

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();

	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);

	FMassEntityHandle NearestEntity;

	TArray<FMassEntityHandle> EntityHandles;
	for (const FMassArchetypeHandle& Archetype : MatchingArchetypes)
	{
		FMassArchetypeEntityCollection Collection(Archetype);
		Collection.ExportEntityHandles(EntityHandles);

		for (const FMassEntityHandle& Entity : EntityHandles)
		{
			if (const FTransformFragment* TransformFrag = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity))
			{
				const FVector EntityPos = TransformFrag->GetTransform().GetLocation();
				//둔각이면 ok 예각이면 no
				double DotResult = FVector::DotProduct(StartLocation - EntityPos, EndLocation - EntityPos);
				if (DotResult < 0)
				{
					OutEntityHandles.Add(Entity);
				}
			}
		}
	}

	if (OutEntityHandles.IsEmpty())
	{
		return false;
	}
	return true;
}


void UTWUnitSubsystem::SpawnUnit(const FVector& Location, const FTWUnitTableRowBase& UnitTableRowBase,
                                 ATWPlayerController* PlayerController)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));

	UWorld* World = GetWorld();
	if (!IsValid(World) || !UnitTableRowBase.MassEntityConfigAsset) return;

	UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!MassSubsystem) return;

	FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();

	const FMassEntityTemplate& EntityTemplate = UnitTableRowBase.MassEntityConfigAsset->
	                                                             GetOrCreateEntityTemplate(*GetWorld());

	TWeakObjectPtr<ATWPlayerController> WeakPlayerController = PlayerController;
	TWeakObjectPtr<ThisClass> WeakThis = this;
	EntityManager.Defer().PushCommand<FMassDeferredCreateCommand>(
		[Location, EntityTemplate, WeakPlayerController, WeakThis, UnitTableRowBase](FMassEntityManager& InOutEntityManager)
		{
			if (false == WeakPlayerController.IsValid())
			{
				return;
			}
			if (false == WeakThis.IsValid())
			{
				return;
			}
			UWorld* World = InOutEntityManager.GetWorld();
			UMassSpawnerSubsystem* MassSpawnerSubsystem = World->GetSubsystem<UMassSpawnerSubsystem>();

			ATWPlayerState* PlayerState = WeakPlayerController->GetPlayerState<ATWPlayerState>();
			if (false == IsValid(PlayerState))
			{
				return;
			}
			TArray<FMassEntityHandle> SpawnedEntities;
			MassSpawnerSubsystem->SpawnEntities(EntityTemplate, 1, SpawnedEntities);

			if (SpawnedEntities.Num() > 0)
			{
				FMassEntityHandle SpawnedUnit = SpawnedEntities[0];


				if (FTWUnitFragment* UnitFragment = InOutEntityManager.GetFragmentDataPtr<FTWUnitFragment>(SpawnedUnit))
				{
					UnitFragment->SetUnitID(UnitTableRowBase.UnitID);
				}

				if (FTransformFragment* TransformFragment = InOutEntityManager.GetFragmentDataPtr<FTransformFragment>(
					SpawnedUnit))
				{
					TransformFragment->GetMutableTransform().SetLocation(Location);
				}
				//TODO Stat HardCoded
				//TODO 유닛 아이디 fragment에 주입
				//TODO 업그레이드 현황에 따른 스탯 설정
				if (FTWStatusFragment* StatusFragment = InOutEntityManager.GetFragmentDataPtr<FTWStatusFragment>(
					SpawnedUnit))
				{
					StatusFragment->SetStatus(UnitTableRowBase.BaseStatus);
				}

				//TODO 유닛 PlayerState에 추가
				// PlayerState->AddUnit(SpawnedUnit);
				WeakThis->AddUnit(WeakPlayerController->GetPlayerState<ATWPlayerState>()->PlayerSlot, SpawnedUnit);
			}
		});
}

TMap<EResourceType, int32> UTWUnitSubsystem::GetUpkeep(int32 PlayerSlot)
{
	return UnitContainers[PlayerSlot]->GetUpkeep();
}

void UTWUnitSubsystem::AddUnit(int32 PlayerSlot, FMassEntityHandle& Unit)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));
	UnitContainers[PlayerSlot]->AddUnit(Unit);
}

void UTWUnitSubsystem::RemoveUnit(int32 PlayerSlot, int32 Idx)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));
	UnitContainers[PlayerSlot]->RemoveUnit(Idx);
}

FTWUnitTableRowBase* UTWUnitSubsystem::GetUnitTableRowBase(FName UnitID) const
{
	return CachedUnitTableRows[UnitID];
}
#endif
