// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/BubbleHandler/TWStatusMassClientBubbleHandler.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Replication/Agent/TWStatusReplicatedAgent.h"
#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FTWStatusMassClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	TArrayView<FTWStatusFragment> StatusFragments;

	// Add the requirements for the query used to grab all the status fragments
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{
		InQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadWrite);
	};

	// Cache the status fragments
	auto CacheFragmentViewsForSpawnQuery = [&]
	(FMassExecutionContext& InExecContext)
	{
		StatusFragments = InExecContext.GetMutableFragmentView<FTWStatusFragment>();
	};

	// Called when a new entity is spawned. Stores the entity status in the status fragment
	auto SetSpawnedEntityData = [&]
	(const FMassEntityView& EntityView, const FTWStatusReplicatedAgent& ReplicatedEntity, const int32 EntityIdx)
	{
		const FTWUnitStatus& StatusData = ReplicatedEntity.GetStatus();
		StatusFragments[EntityIdx].GetMutableStatus().Status = StatusData.Status;
	};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FTWStatusReplicatedAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

}

void FTWStatusMassClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	PostReplicatedChangeHelper(
		ChangedIndices,
		[this](const FMassEntityView& EntityView, const FTWStatusReplicatedAgent& Item)
		{
			PostReplicatedChangeEntity(EntityView, Item);
		});
}

void FTWStatusMassClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView,
                                                          const FTWStatusReplicatedAgent& Item)
{
	// Grabs the status fragment from the entity  
	FTWStatusFragment& StatusFragment = EntityView.GetFragmentData<FTWStatusFragment>();

	// Sets the status with the agent status  
	const FTWUnitStatus& StatusData = Item.GetStatus();
	StatusFragment.GetMutableStatus().Status = StatusData.Status;
}

#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
