// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/BubbleHandler/TWTransformMassClientBubbleHandler.h"

#include "MassCommonFragments.h"
#include "Mass/Replication/Agent/TWTransformReplicatedAgent.h"
#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FTWTransformMassClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	TArrayView<FTransformFragment> TransformFragments;

	// Add the requirements for the query used to grab all the transform fragments
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{
		InQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	};

	// Cache the transform fragments
	auto CacheFragmentViewsForSpawnQuery = [&]
	(FMassExecutionContext& InExecContext)
	{
		TransformFragments = InExecContext.GetMutableFragmentView<FTransformFragment>();
	};

	// Called when a new entity is spawned. Stores the entity location in the transform fragment
	auto SetSpawnedEntityData = [&]
	(const FMassEntityView& EntityView, const FTWTransformReplicatedAgent& ReplicatedEntity, const int32 EntityIdx)
	{
		const FReplicatedAgentPositionYawData& PositionYawData = ReplicatedEntity.GetPositionYawData();
		TransformFragments[EntityIdx].GetMutableTransform().SetLocation(PositionYawData.GetPosition());
		TransformFragments[EntityIdx].GetMutableTransform().SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(PositionYawData.GetYaw())));
		
	};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FTWTransformReplicatedAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

}

void FTWTransformMassClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	PostReplicatedChangeHelper(
		ChangedIndices,
		[this](const FMassEntityView& EntityView, const FTWTransformReplicatedAgent& Item)
		{
			PostReplicatedChangeEntity(EntityView, Item);
		});
}

void FTWTransformMassClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView,
                                                          const FTWTransformReplicatedAgent& Item)
{
	// Grabs the transform fragment from the entity  
	FTransformFragment& TransformFragment = EntityView.GetFragmentData<FTransformFragment>();

	// Sets the transform location with the agent location  
	const FReplicatedAgentPositionYawData& PositionYawData = Item.GetPositionYawData();
	TransformFragment.GetMutableTransform().SetLocation(PositionYawData.GetPosition());
	TransformFragment.GetMutableTransform().SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(PositionYawData.GetYaw())));
	
}

#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
