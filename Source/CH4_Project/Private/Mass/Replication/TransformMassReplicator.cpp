// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/TransformMassReplicator.h"

#include "Mass/Replication/Agent/TransformReplicatedAgent.h"
#include "Mass/Replication/BubbleHandler/TransformMassClientBubbleHandler.h"
#include "Mass/Replication/BubbleInfo/TransformMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleSerializer/TransformClientBubbleSerializer.h"
#include "Mass/Replication/FastArrayItem/TransformMassFastArrayItem.h"

void UTransformMassReplicator::ProcessClientReplication(FMassExecutionContext& Context,
                                                        FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;  
	TConstArrayView<FTransformFragment> TransformFragments;
 
	auto CacheViewsCallback = [&] (FMassExecutionContext& InContext)  
	{  
		TransformFragments = InContext.GetFragmentView<FTransformFragment>();  
		RepSharedFrag = &InContext.GetMutableSharedFragment<FMassReplicationSharedFragment>();  
	};
	
	auto AddEntityCallback = [&] (FMassExecutionContext& InContext, const int32 EntityIdx, FTransformReplicatedAgent& InReplicatedAgent, const FMassClientHandle ClientHandle)  
	{  
		// Retrieves the bubble of the relevant client
		ATransformMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATransformMassClientBubbleInfo>(ClientHandle);  
 
		
		FReplicatedAgentPositionYawData PositionYawData;
		PositionYawData.SetPosition(TransformFragments[EntityIdx].GetTransform().GetLocation());
		PositionYawData.SetYaw(FRotator::NormalizeAxis( TransformFragments[EntityIdx].GetTransform().Rotator().Yaw));
		InReplicatedAgent.SetPositionYawData(PositionYawData);
 
		// Adds the new agent in the client bubble
		return BubbleInfo.GetBubbleSerializer().Bubble.AddAgent(InContext.GetEntity(EntityIdx), InReplicatedAgent);  
	};
	
	auto ModifyEntityCallback = [&]  
(FMassExecutionContext& InContext, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)  
	{  
		// Grabs the client bubble
		ATransformMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATransformMassClientBubbleInfo>(ClientHandle);  
		FTransformMassClientBubbleHandler& Bubble = BubbleInfo.GetBubbleSerializer().Bubble;  
 
		// Retrieves the entity agent
		FTransformMassFastArrayItem* Item = Bubble.GetMutableItem(Handle);  
    
		bool bMarkItemDirty = false;  
    
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

		if (bMarkItemDirty)  
		{  
			// Marks the agent as dirty so it replicated to the client
			Bubble.MarkItemDirty(*Item);  
		}  
	};
	
	auto RemoveEntityCallback = [RepSharedFrag](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)  
	{  
		// Retrieve the client bubble
		ATransformMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATransformMassClientBubbleInfo>(ClientHandle); 
 
		// Remove the entity agent from the bubble
		BubbleInfo.GetBubbleSerializer().Bubble.RemoveAgent(Handle);  
	};
	
	CalculateClientReplication<FTransformMassFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
