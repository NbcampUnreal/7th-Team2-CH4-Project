// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/BubbleHandler/TWMassClientBubbleHandler.h"

#include "MassMovementFragments.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Fragments/TWTransformOffsetParams.h"
#include "MassReplicationTransformHandlers.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Replication/Agent/TWReplicatedAgent.h"
#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FTWMassClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	TArrayView<FTWStatusFragment> StatusFragments;
	TArrayView<FTransformFragment> TransformFragments;
	TArrayView<FTWUnitFragment> UnitFragments;
	TArrayView<FTWAttackFragment> AttackFragments;
	TArrayView<FMassVelocityFragment> VelocityFragments;
	
	// Add the requirements for the query used to grab all the status fragments
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{
		InQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FTWUnitFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	};

	// Cache the status fragments
	auto CacheFragmentViewsForSpawnQuery = [&]
	(FMassExecutionContext& InExecContext)
	{
		StatusFragments = InExecContext.GetMutableFragmentView<FTWStatusFragment>();
		TransformFragments = InExecContext.GetMutableFragmentView<FTransformFragment>();
		UnitFragments = InExecContext.GetMutableFragmentView<FTWUnitFragment>();
		AttackFragments = InExecContext.GetMutableFragmentView<FTWAttackFragment>();
		VelocityFragments = InExecContext.GetMutableFragmentView<FMassVelocityFragment>();
	};

	// Called when a new entity is spawned. Stores the entity status in the status fragment
	auto SetSpawnedEntityData = [&]
	(const FMassEntityView& EntityView, const FTWReplicatedAgent& ReplicatedEntity, const int32 EntityIdx)
	{
		const FTWUnitStatus& StatusData = ReplicatedEntity.GetStatus();
		// StatusFragments[EntityIdx].GetMutableStatus().Status = StatusData.Status;
		for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); i++)
		{
			StatusFragments[EntityIdx].GetMutableStatus().Status[i] = StatusData.Status[i];
		}
		
		
		const FReplicatedAgentPositionYawData& PositionYawData = ReplicatedEntity.GetPositionYawData();
		TransformFragments[EntityIdx].GetMutableTransform().SetLocation(PositionYawData.GetPosition());
		TransformFragments[EntityIdx].GetMutableTransform().SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(PositionYawData.GetYaw())));
		
		const int32 Owner = ReplicatedEntity.GetOwner();
		const FName UnitID = ReplicatedEntity.GetUnitID();
		UnitFragments[EntityIdx].SetOwner(Owner);
		UnitFragments[EntityIdx].SetUnitID(UnitID);
		
		const float LastAttackTime = ReplicatedEntity.GetLastAttackTime();
		AttackFragments[EntityIdx].LastAttackTime = LastAttackTime;
		
		VelocityFragments[EntityIdx].Value = ReplicatedEntity.GetVelocity();
	};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FTWReplicatedAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

}

void FTWMassClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	PostReplicatedChangeHelper(
		ChangedIndices,
		[this](const FMassEntityView& EntityView, const FTWReplicatedAgent& Item)
		{
			PostReplicatedChangeEntity(EntityView, Item);
		});
}

void FTWMassClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView,
                                                          const FTWReplicatedAgent& Item)
{
	// Grabs the status fragment from the entity  
	FTWStatusFragment& StatusFragment = EntityView.GetFragmentData<FTWStatusFragment>();

	// Sets the status with the agent status  
	const FTWUnitStatus& StatusData = Item.GetStatus();
	//StatusFragment.GetMutableStatus().Status = StatusData.Status;
	for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); i++)
	{
		StatusFragment.GetMutableStatus().Status[i] = StatusData.Status[i];
	}
	
	FTransformFragment& TransformFragment = EntityView.GetFragmentData<FTransformFragment>();
  	
	const FVector PreviousLocation = TransformFragment.GetTransform().GetLocation();
	const float PreviousYaw = FRotator::NormalizeAxis(TransformFragment.GetTransform().Rotator().Yaw);
	
	const FReplicatedAgentPositionYawData& PositionYawData = Item.GetPositionYawData();
	TransformFragment.GetMutableTransform().SetLocation(PositionYawData.GetPosition());
	TransformFragment.GetMutableTransform().SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(PositionYawData.GetYaw())));
	
	const FVector NewLocation = TransformFragment.GetTransform().GetLocation();
	const float NewYaw = FRotator::NormalizeAxis(TransformFragment.GetTransform().Rotator().Yaw);
  
 
	// Initializes mesh offset 
	FTWTransformOffsetFragment& TranslationOffset = EntityView.GetFragmentData<FTWTransformOffsetFragment>();
	const FTWTransformOffsetParams& OffsetParams = EntityView.GetConstSharedFragmentData<FTWTransformOffsetParams>();
  	
	if (OffsetParams.MaxSmoothNetUpdateDistanceSqr > FVector::DistSquared(PreviousLocation, NewLocation))
	{
		// Offsetting the mesh to sync with the sever locations smoothly
		TranslationOffset.TransformOffset.SetPosition( TranslationOffset.TransformOffset.GetPosition()+ PreviousLocation - NewLocation);
	}
	
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(NewYaw, PreviousYaw);
	if (OffsetParams.MaxSmoothNetUpdateYaw > FMath::Abs(DeltaYaw))
	{
		float CurrentYawOffset = TranslationOffset.TransformOffset.GetYaw();
		TranslationOffset.TransformOffset.SetYaw(FRotator::NormalizeAxis(CurrentYawOffset + DeltaYaw));
	}
	
	
	// Grabs the status fragment from the entity  
	FTWUnitFragment& UnitFragment = EntityView.GetFragmentData<FTWUnitFragment>();

	// Sets the status with the agent status  
	const int32 Owner = Item.GetOwner();
	const FName UnitID = Item.GetUnitID();
	UnitFragment.SetOwner(Owner);
	UnitFragment.SetUnitID(UnitID);
	
	FTWAttackFragment& AttackFragment = EntityView.GetFragmentData<FTWAttackFragment>();
	
	const float LastAttackTime = Item.GetLastAttackTime();
	AttackFragment.LastAttackTime = LastAttackTime;
	
	FMassVelocityFragment& VelocityFragment = EntityView.GetFragmentData<FMassVelocityFragment>();
	VelocityFragment.Value = Item.GetVelocity();
	
				
	UE_LOG(LogTemp,Warning,TEXT("%lf, %lf : %lf"), 		VelocityFragment.Value.X,		VelocityFragment.Value.Y	,	VelocityFragment.Value.Size2D())

}

#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
