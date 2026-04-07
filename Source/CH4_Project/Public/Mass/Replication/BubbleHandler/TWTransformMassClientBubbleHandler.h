// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleHandler.h"
#include "MassCommonFragments.h"
#include "Mass/Replication/Agent/TWTransformReplicatedAgent.h"
#include "Mass/Replication/FastArrayItem/TWTransformMassFastArrayItem.h"

/**
 * 
 */
class CH4_PROJECT_API FTWTransformMassClientBubbleHandler : public TClientBubbleHandlerBase<FTWTransformMassFastArrayItem>
{
public:  
#if UE_REPLICATION_COMPILE_SERVER_CODE  
 
	/** Returns the item containing the agent with given handle */
	FTWTransformMassFastArrayItem* GetMutableItem(FMassReplicatedAgentHandle Handle) 
	{
		if (AgentHandleManager.IsValidHandle(Handle))
		{
			const FMassAgentLookupData& LookUpData = AgentLookupArray[Handle.GetIndex()];
			return &(*Agents)[LookUpData.AgentsIdx];
		}
		return nullptr;
	}
 
	/** Marks the given item as modified so it replicates its changes to th client */
	void MarkItemDirty(FTWTransformMassFastArrayItem & Item) const 
	{
		Serializer->MarkItemDirty(Item);
	}
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
	
#if UE_REPLICATION_COMPILE_CLIENT_CODE	
	virtual void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) override;
	virtual void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) override;
	virtual void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FTWTransformReplicatedAgent& Item);  
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE
};
