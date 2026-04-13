// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/TWStatusMassReplicator.h"

#include "Mass/Replication/Agent/TWStatusReplicatedAgent.h"
#include "Mass/Replication/BubbleHandler/TWStatusMassClientBubbleHandler.h"
#include "Mass/Replication/BubbleInfo/TWStatusMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleSerializer/TWStatusClientBubbleSerializer.h"
#include "Mass/Replication/FastArrayItem/TWStatusMassFastArrayItem.h"

void UTWStatusMassReplicator::ProcessClientReplication(FMassExecutionContext& Context,
                                                          FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;  
	TConstArrayView<FTWStatusFragment> StatusFragments;
 
	auto CacheViewsCallback = [&] (FMassExecutionContext& InContext)  
	{  
		StatusFragments = InContext.GetFragmentView<FTWStatusFragment>();  
		RepSharedFrag = &InContext.GetMutableSharedFragment<FMassReplicationSharedFragment>();  
	};
	
	auto AddEntityCallback = [&] (FMassExecutionContext& InContext, const int32 EntityIdx, FTWStatusReplicatedAgent& InReplicatedAgent, const FMassClientHandle ClientHandle)  
	{  
		// Retrieves the bubble of the relevant client
		ATWStatusMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWStatusMassClientBubbleInfo>(ClientHandle);  
 
		
		FTWUnitStatus StatusData;
		StatusData.Status = StatusFragments[EntityIdx].GetStatus().Status;
		InReplicatedAgent.SetStatus(StatusData);		
 
		// Adds the new agent in the client bubble
		return BubbleInfo.GetBubbleSerializer().Bubble.AddAgent(InContext.GetEntity(EntityIdx), InReplicatedAgent);  
	};
	
	auto ModifyEntityCallback = [&]  
(FMassExecutionContext& InContext, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)  
	{  
		// Grabs the client bubble
		ATWStatusMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWStatusMassClientBubbleInfo>(ClientHandle);  
		FTWStatusMassClientBubbleHandler& Bubble = BubbleInfo.GetBubbleSerializer().Bubble;  
 
		// Retrieves the entity agent
		FTWStatusMassFastArrayItem* Item = Bubble.GetMutableItem(Handle);  
    
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

		if (bMarkItemDirty)  
		{  
			// Marks the agent as dirty so it replicated to the client
			Bubble.MarkItemDirty(*Item);  
		}  
	};
	
	auto RemoveEntityCallback = [RepSharedFrag](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)  
	{  
		// Retrieve the client bubble
		ATWStatusMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWStatusMassClientBubbleInfo>(ClientHandle); 
 
		// Remove the entity agent from the bubble
		BubbleInfo.GetBubbleSerializer().Bubble.RemoveAgent(Handle);  
	};
	
	CalculateClientReplication<FTWStatusMassFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
