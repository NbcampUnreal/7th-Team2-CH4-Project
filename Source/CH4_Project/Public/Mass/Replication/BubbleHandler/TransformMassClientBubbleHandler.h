// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleHandler.h"
#include "MassCommonFragments.h"
#include "Mass/Replication/Agent/TransformReplicatedAgent.h"
#include "Mass/Replication/FastArrayItem/TransformMassFastArrayItem.h"

/**
 * 
 */
class CH4_PROJECT_API FTransformMassClientBubbleHandler : public TClientBubbleHandlerBase<FTransformMassFastArrayItem>
{
public:  
#if UE_REPLICATION_COMPILE_SERVER_CODE  
 
	/** Returns the item containing the agent with given handle */
	FTransformMassFastArrayItem* GetMutableItem(FMassReplicatedAgentHandle Handle) 
	{
		if (AgentHandleManager.IsValidHandle(Handle))
		{
			const FMassAgentLookupData& LookUpData = AgentLookupArray[Handle.GetIndex()];
			return &(*Agents)[LookUpData.AgentsIdx];
		}
		return nullptr;
	}
 
	/** Marks the given item as modified so it replicates its changes to th client */
	void MarkItemDirty(FTransformMassFastArrayItem & Item) const 
	{
		Serializer->MarkItemDirty(Item);
	}
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
	
#if UE_REPLICATION_COMPILE_CLIENT_CODE	
	virtual void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) override;
	virtual void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) override;
	virtual void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FTransformReplicatedAgent& Item);  
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE
};
