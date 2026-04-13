// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonFragments.h"
#include "MassReplicationProcessor.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "TWStatusMassReplicator.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API UTWStatusMassReplicator : public UMassReplicatorBase
{
	GENERATED_BODY()

public:
	/** Adds the replicated fragments to the query as requirements */
	virtual void AddRequirements(FMassEntityQuery& EntityQuery) override
	{
		EntityQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadOnly);
	}

	virtual void ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext) override;
};
