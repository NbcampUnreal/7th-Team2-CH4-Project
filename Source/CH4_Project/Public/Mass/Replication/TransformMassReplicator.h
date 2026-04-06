// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonFragments.h"
#include "MassReplicationProcessor.h"
#include "TransformMassReplicator.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API UTransformMassReplicator : public UMassReplicatorBase
{
	GENERATED_BODY()

public:
	/** Adds the replicated fragments to the query as requirements */
	virtual void AddRequirements(FMassEntityQuery& EntityQuery) override
	{
		EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	}

	virtual void ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext) override;
};
