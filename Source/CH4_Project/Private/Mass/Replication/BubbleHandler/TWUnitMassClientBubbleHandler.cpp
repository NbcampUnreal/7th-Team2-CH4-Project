// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/BubbleHandler/TWUnitMassClientBubbleHandler.h"
#include "Mass/Fragments/TWUnitFragment.h"
#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FTWUnitMassClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	TArrayView<FTWUnitFragment> UnitFragments;

	// Add the requirements for the query used to grab all the status fragments
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{
		InQuery.AddRequirement<FTWUnitFragment>(EMassFragmentAccess::ReadWrite);
	};

	// Cache the status fragments
	auto CacheFragmentViewsForSpawnQuery = [&]
	(FMassExecutionContext& InExecContext)
	{
		UnitFragments = InExecContext.GetMutableFragmentView<FTWUnitFragment>();
	};

	// Called when a new entity is spawned. Stores the entity status in the status fragment
	auto SetSpawnedEntityData = [&]
	(const FMassEntityView& EntityView, const FTWUnitReplicatedAgent& ReplicatedEntity, const int32 EntityIdx)
	{
		const int32 Owner = ReplicatedEntity.GetOwner();
		const FName UnitID = ReplicatedEntity.GetUnitID();
		UnitFragments[EntityIdx].SetOwner(Owner);
		UnitFragments[EntityIdx].SetUnitID(UnitID);
	};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FTWUnitReplicatedAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

}

void FTWUnitMassClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	PostReplicatedChangeHelper(
		ChangedIndices,
		[this](const FMassEntityView& EntityView, const FTWUnitReplicatedAgent& Item)
		{
			PostReplicatedChangeEntity(EntityView, Item);
		});
}

void FTWUnitMassClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView,
                                                          const FTWUnitReplicatedAgent& Item)
{
	// Grabs the status fragment from the entity  
	FTWUnitFragment& UnitFragment = EntityView.GetFragmentData<FTWUnitFragment>();

	// Sets the status with the agent status  
	const int32 Owner = Item.GetOwner();
	const FName UnitID = Item.GetUnitID();
	UnitFragment.SetOwner(Owner);
	UnitFragment.SetUnitID(UnitID);
}

#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
