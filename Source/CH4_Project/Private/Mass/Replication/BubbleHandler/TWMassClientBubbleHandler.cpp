// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/BubbleHandler/TWMassClientBubbleHandler.h"

#include "MassMovementFragments.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Fragments/TWTransformOffsetParams.h"
#include "MassReplicationTransformHandlers.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWClientVelocityFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Replication/Agent/TWReplicatedAgent.h"
#include "Runtime/MassEntity/Internal/MassArchetypeData.h"
#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FTWMassClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	TArrayView<FTWStatusFragment> StatusFragments;
	TArrayView<FTransformFragment> TransformFragments;
	TArrayView<FTWUnitFragment> UnitFragments;
	TArrayView<FTWAttackFragment> AttackFragments;
	TArrayView<FTWClientVelocityFragment> VelocityFragments;
	
	// Add the requirements for the query used to grab all the status fragments
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{
		InQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FTWUnitFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);
		InQuery.AddRequirement<FTWClientVelocityFragment>(EMassFragmentAccess::ReadWrite);
	};

	// Cache the status fragments
	auto CacheFragmentViewsForSpawnQuery = [&]
	(FMassExecutionContext& InExecContext)
	{
		StatusFragments = InExecContext.GetMutableFragmentView<FTWStatusFragment>();
		TransformFragments = InExecContext.GetMutableFragmentView<FTransformFragment>();
		UnitFragments = InExecContext.GetMutableFragmentView<FTWUnitFragment>();
		AttackFragments = InExecContext.GetMutableFragmentView<FTWAttackFragment>();
		VelocityFragments = InExecContext.GetMutableFragmentView<FTWClientVelocityFragment>();
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
		StatusFragments[EntityIdx].SetIsDeath(ReplicatedEntity.GetIsDeath());
		StatusFragments[EntityIdx].SetDestroyTime(ReplicatedEntity.GetDestroyTime());
		
		
		const FReplicatedAgentPositionYawData& PositionYawData = ReplicatedEntity.GetPositionYawData();
		TransformFragments[EntityIdx].GetMutableTransform().SetLocation(PositionYawData.GetPosition());
		TransformFragments[EntityIdx].GetMutableTransform().SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(PositionYawData.GetYaw())));
		
		const int32 Owner = ReplicatedEntity.GetOwner();
		const FName UnitID = ReplicatedEntity.GetUnitID();
		UnitFragments[EntityIdx].SetOwner(Owner);
		UnitFragments[EntityIdx].SetUnitID(UnitID);
		
		const float LastAttackTime = ReplicatedEntity.GetLastAttackTime();
		AttackFragments[EntityIdx].LastAttackTime = LastAttackTime;
		
		VelocityFragments[EntityIdx].Velocity = ReplicatedEntity.GetVelocity();
	};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FTWReplicatedAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

}

void FTWMassClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{

	FMassEntityManager& EntityManager = Serializer->GetEntityManagerChecked();

	UMassReplicationSubsystem* ReplicationSubsystem = Serializer->GetReplicationSubsystem();
	check(ReplicationSubsystem);

	// Go through the changed Entities and update their Mass data
	for (int32 Idx : ChangedIndices)
	{
		const FTWMassFastArrayItem& ChangedItem = (*Agents)[Idx];

		const FMassReplicationEntityInfo* EntityInfo = ReplicationSubsystem->FindMassEntityInfo(ChangedItem.Agent.GetNetID());

		checkf(EntityInfo, TEXT("EntityInfo must be valid if the Agent has already been added (which it must have been to get PostReplicatedChange"));
		checkf(EntityInfo->ReplicationID >= ChangedItem.ReplicationID, TEXT("ReplicationID out of sync, this should never happen!"));

		// Currently we don't think this should be needed, but are leaving it in for bomb proofing.
		if (ensure(EntityInfo->ReplicationID == ChangedItem.ReplicationID))
		{
			const FMassArchetypeHandle ArchetypeHandle = EntityManager.GetArchetypeForEntity(EntityInfo->Entity);
			if (!FMassArchetypeHelper::ArchetypeDataFromHandle(ArchetypeHandle))
			{
				continue;
			}
			
			FMassEntityView EntityView(EntityManager, EntityInfo->Entity);
			PostReplicatedChangeEntity(EntityView, ChangedItem.Agent);
		}
	}
	
	
	
	
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
	StatusFragment.SetIsDeath(Item.GetIsDeath());
	StatusFragment.SetDestroyTime(Item.GetDestroyTime());
	
	
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
	
	FTWClientVelocityFragment& VelocityFragment = EntityView.GetFragmentData<FTWClientVelocityFragment>();
	VelocityFragment.Velocity = Item.GetVelocity();
}

#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
