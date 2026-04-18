// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWDespawnProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "MassSignalSubsystem.h"
#include "Mass/TWUnit.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Traits/TWCommandTrait.h"
#include "Subsystems/TWUnitSubsystem.h"

UTWDespawnProcessor::UTWDespawnProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
	
	bRequiresGameThreadExecution = true;
}

void UTWDespawnProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
}

void UTWDespawnProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [&EntityManager](FMassExecutionContext& Context)
	{
		const TArrayView<FTWStatusFragment> StatusList = Context.GetMutableFragmentView<FTWStatusFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		double TimeSeconds = Context.GetWorld()->GetTimeSeconds();
		TArray<FMassEntityHandle> EntitiesToDestroy;

		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			
			FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
			if (StatusList[EntityIdx].GetDestroyTime() < TimeSeconds)
			{
				EntitiesToDestroy.Add(Entity);
				//Context.Defer().RemoveTag<FTWMassDeadTag>(Entity);
				
			}
		}
		Context.Defer().DestroyEntities(EntitiesToDestroy);
		
	});
}
