// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonFragments.h"
#include "MassReplicationProcessor.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "TWMassReplicator.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API UTWMassReplicator : public UMassReplicatorBase
{
	GENERATED_BODY()

public:
	/** Adds the replicated fragments to the query as requirements */
	virtual void AddRequirements(FMassEntityQuery& EntityQuery) override
	{
		EntityQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadOnly);
		EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
		EntityQuery.AddRequirement<FTWUnitFragment>(EMassFragmentAccess::ReadOnly);
		EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadOnly);
		
		
	}

	virtual void ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext) override;
};
