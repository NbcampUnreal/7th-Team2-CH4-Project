// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/TWUnitSubsystem.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "MassNavigationSubsystem.h"
#include "MassReplicationSubsystem.h"
#include "Mass/Replication/BubbleInfo/TWTransformMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleInfo/TWTransformSmoothMassClientBubbleInfo.h"

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
}

void UTWUnitSubsystem::PostInitialize()
{
	Super::PostInitialize();
	const UWorld* World = GetWorld();
	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(World);
	ReplicationSubsystem->RegisterBubbleInfoClass(ATWTransformMassClientBubbleInfo::StaticClass());
	ReplicationSubsystem->RegisterBubbleInfoClass(ATWTransformSmoothMassClientBubbleInfo::StaticClass());
}

//TODO FMassEntityHandle을 이 시스템에서 별도로 관리하고 Spatial Hasing적용해서 순회해야함
//현재는 단순히 월드에 존재하는 모든 엔티티를 조회함.
bool UTWUnitSubsystem::FindNearestEntity(const FVector& Location, FMassEntityHandle& OutEntityHandle, float MaxDistance)
{
	UMassEntitySubsystem* EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return false;

	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
    
	TArray<FMassArchetypeHandle> MatchingArchetypes;
	EntityManager.GetMatchingArchetypes(FindNearestEntityQuery, MatchingArchetypes);
	
	float MinSquaredDistance = FMath::Square(MaxDistance);
	FMassEntityHandle NearestEntity;
	
	TArray<FMassEntityHandle> EntityHandles ;
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