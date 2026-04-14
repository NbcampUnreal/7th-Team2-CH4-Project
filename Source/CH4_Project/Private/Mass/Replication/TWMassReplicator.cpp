// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/TWMassReplicator.h"

#include "MassReplicationTransformHandlers.h"
#include "Mass/Replication/Agent/TWReplicatedAgent.h"
#include "Mass/Replication/BubbleHandler/TWMassClientBubbleHandler.h"
#include "Mass/Replication/BubbleInfo/TWMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleSerializer/TWClientBubbleSerializer.h"
#include "Mass/Replication/FastArrayItem/TWMassFastArrayItem.h"

void UTWMassReplicator::ProcessClientReplication(FMassExecutionContext& Context,
                                                          FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;  
	TConstArrayView<FTWStatusFragment> StatusFragments;
	TConstArrayView<FTransformFragment> TransformFragments;
	TConstArrayView<FTWUnitFragment> UnitFragments;
	
	auto CacheViewsCallback = [&] (FMassExecutionContext& InContext)  
	{  
		StatusFragments = InContext.GetFragmentView<FTWStatusFragment>();  
		TransformFragments = InContext.GetFragmentView<FTransformFragment>();  
		UnitFragments = InContext.GetFragmentView<FTWUnitFragment>();  
		RepSharedFrag = &InContext.GetMutableSharedFragment<FMassReplicationSharedFragment>();  
	};
	
	auto AddEntityCallback = [&] (FMassExecutionContext& InContext, const int32 EntityIdx, FTWReplicatedAgent& InReplicatedAgent, const FMassClientHandle ClientHandle)  
	{  
		// Retrieves the bubble of the relevant client
		ATWMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWMassClientBubbleInfo>(ClientHandle);  
 
		
		FTWUnitStatus StatusData;
		// StatusData.Status = StatusFragments[EntityIdx].GetStatus().Status;
		
		for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); i++)
		{
			StatusData.Status[i] = StatusFragments[EntityIdx].GetStatus().Status[i];
		}
		
		InReplicatedAgent.SetStatus(StatusData);		
 
		FReplicatedAgentPositionYawData PositionYawData;
		PositionYawData.SetPosition(TransformFragments[EntityIdx].GetTransform().GetLocation());
		PositionYawData.SetYaw(FRotator::NormalizeAxis( TransformFragments[EntityIdx].GetTransform().Rotator().Yaw));
		InReplicatedAgent.SetPositionYawData(PositionYawData);
		
		int32 Owner = UnitFragments[EntityIdx].GetOwner();
		InReplicatedAgent.SetOwner(Owner);
		FName UnitID = UnitFragments[EntityIdx].GetUnitID();
		InReplicatedAgent.SetUnitID(UnitID);
		
		// Adds the new agent in the client bubble
		return BubbleInfo.GetBubbleSerializer().Bubble.AddAgent(InContext.GetEntity(EntityIdx), InReplicatedAgent);  
	};
	
	auto ModifyEntityCallback = [&]  
(FMassExecutionContext& InContext, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)  
	{  
		// Grabs the client bubble
		ATWMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWMassClientBubbleInfo>(ClientHandle);  
		FTWMassClientBubbleHandler& Bubble = BubbleInfo.GetBubbleSerializer().Bubble;  
 
		// Retrieves the entity agent
		FTWMassFastArrayItem* Item = Bubble.GetMutableItem(Handle);  
    
		bool bMarkItemDirty = false;  
    
		const FTWUnitStatus& EntityStatus = StatusFragments[EntityIdx].GetStatus();
		for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); i++)
		{
			if (EntityStatus.Status[i] != Item->Agent.GetStatus().Status[i])
			{
				Item->Agent.SetStatus(EntityStatus);
				bMarkItemDirty = true;  
				break;
			}
		}

		const FVector& EntityLocation = TransformFragments[EntityIdx].GetTransform().GetLocation();  
		constexpr float LocationTolerance = 10.0f;  
		const float EntityYaw = TransformFragments[EntityIdx].GetTransform().Rotator().Yaw;
		constexpr float YawTolerance = 10.0f;
		if (!FVector::PointsAreNear(EntityLocation, Item->Agent.GetPositionYawData().GetPosition(), LocationTolerance)||
		!FMath::IsNearlyEqual(EntityYaw,Item->Agent.GetPositionYawData().GetYaw(),YawTolerance)
		)  
		{  
			FReplicatedAgentPositionYawData PositionYawData;
			PositionYawData.SetPosition(EntityLocation);
			PositionYawData.SetYaw(FRotator::NormalizeAxis( TransformFragments[EntityIdx].GetTransform().Rotator().Yaw));
			
			Item->Agent.SetPositionYawData(PositionYawData);
			bMarkItemDirty = true;  
		}
		
		const int32 Owner = UnitFragments[EntityIdx].GetOwner();
		const FName UnitID = UnitFragments[EntityIdx].GetUnitID();
		if (Owner != Item->Agent.GetOwner() || UnitID != Item->Agent.GetUnitID())
		{
			Item->Agent.SetOwner(Owner);
			Item->Agent.SetUnitID(UnitID);
			bMarkItemDirty = true;
		}
		
		if (bMarkItemDirty)  
		{  
			// Marks the agent as dirty so it replicated to the client
			Bubble.MarkItemDirty(*Item);  
		}  
	};
	
	auto RemoveEntityCallback = [RepSharedFrag](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)  
	{  
		// Retrieve the client bubble
		ATWMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWMassClientBubbleInfo>(ClientHandle); 
 
		// Remove the entity agent from the bubble
		BubbleInfo.GetBubbleSerializer().Bubble.RemoveAgent(Handle);  
	};
	
	CalculateClientReplication<FTWMassFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
