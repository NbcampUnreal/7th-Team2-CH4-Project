// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWAttackAnimationProcessor.h"

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

UTWAttackAnimationProcessor::UTWAttackAnimationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client | (int32)EProcessorExecutionFlags::Standalone;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::SyncWorldToMass);
	
	bRequiresGameThreadExecution = true;
}

void UTWAttackAnimationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
}

void UTWAttackAnimationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	
	EntityQuery.ForEachEntityChunk(Context, [&EntityManager](FMassExecutionContext& Context)
	{
		const TArrayView<FTWAttackFragment> AttackList = Context.GetMutableFragmentView<FTWAttackFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		
		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			if(AttackList[EntityIdx].LastAttackAnimationTime < AttackList[EntityIdx].LastAttackTime)
			{
				AttackList[EntityIdx].LastAttackAnimationTime = AttackList[EntityIdx].LastAttackTime;
				if (ATWUnit* Unit = Cast<ATWUnit>(ActorList[EntityIdx].GetMutable()))
				{
					Unit->PlayAttackMontage();
				}
				
			}			
		}

	});
}
