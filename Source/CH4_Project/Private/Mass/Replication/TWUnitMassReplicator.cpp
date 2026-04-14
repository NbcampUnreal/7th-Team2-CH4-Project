// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/TWUnitMassReplicator.h"

#include "Mass/Replication/Agent/TWUnitReplicatedAgent.h"
#include "Mass/Replication/BubbleHandler/TWUnitMassClientBubbleHandler.h"
#include "Mass/Replication/BubbleInfo/TWUnitMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleSerializer/TWUnitClientBubbleSerializer.h"
#include "Mass/Replication/FastArrayItem/TWUnitMassFastArrayItem.h"

void UTWUnitMassReplicator::ProcessClientReplication(FMassExecutionContext& Context,
                                                          FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;  
	TConstArrayView<FTWUnitFragment> UnitFragments;
 
	auto CacheViewsCallback = [&] (FMassExecutionContext& InContext)  
	{  
		UnitFragments = InContext.GetFragmentView<FTWUnitFragment>();  
		RepSharedFrag = &InContext.GetMutableSharedFragment<FMassReplicationSharedFragment>();  
	};
	
	auto AddEntityCallback = [&] (FMassExecutionContext& InContext, const int32 EntityIdx, FTWUnitReplicatedAgent& InReplicatedAgent, const FMassClientHandle ClientHandle)  
	{  
		// Retrieves the bubble of the relevant client
		ATWUnitMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWUnitMassClientBubbleInfo>(ClientHandle);  
 
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
		ATWUnitMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWUnitMassClientBubbleInfo>(ClientHandle);  
		FTWUnitMassClientBubbleHandler& Bubble = BubbleInfo.GetBubbleSerializer().Bubble;  
 
		// Retrieves the entity agent
		FTWUnitMassFastArrayItem* Item = Bubble.GetMutableItem(Handle);  
    
		bool bMarkItemDirty = false;  
    
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
		ATWUnitMassClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<ATWUnitMassClientBubbleInfo>(ClientHandle); 
 
		// Remove the entity agent from the bubble
		BubbleInfo.GetBubbleSerializer().Bubble.RemoveAgent(Handle);  
	};
	
	CalculateClientReplication<FTWUnitMassFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
