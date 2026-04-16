// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWTargetSearchProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Traits/TWCommandTrait.h"
#include "Subsystems/TWUnitSubsystem.h"

UTWTargetSearchProcessor::UTWTargetSearchProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server | (int32)EProcessorExecutionFlags::Standalone;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::SyncWorldToMass);
	
	bRequiresGameThreadExecution = true;
}

void UTWTargetSearchProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWUnitFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FTWMassSearchingTag>(EMassFragmentPresence::All);
}

void UTWTargetSearchProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		constexpr float SearchingInterval = 0.1f;
		
		UTWUnitSubsystem* UnitSubsystem = Context.GetWorld()->GetSubsystem<UTWUnitSubsystem>();
		if (false == IsValid(UnitSubsystem))
		{
			return;
		}

		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FTWStatusFragment> StatusList = Context.GetFragmentView<FTWStatusFragment>();
		const TConstArrayView<FTWUnitFragment> UnitList = Context.GetFragmentView<FTWUnitFragment>();
		const TArrayView<FTWAttackFragment> AttackList = Context.GetMutableFragmentView<FTWAttackFragment>();
		
		double TimeSeconds = Context.GetWorld()->GetTimeSeconds();
		
		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			if (AttackList[EntityIdx].bIsTargetSet)
			{
				continue;
			}
			if (TimeSeconds - AttackList[EntityIdx].LastAttackTime < SearchingInterval)
			{
				continue;
			}
			UE_LOG(LogTemp, Warning, TEXT("Entity %d Is Searching "), EntityIdx)	
			FVector EntityLocation = TransformList[EntityIdx].GetTransform().GetLocation();
			FMassEntityHandle Target;
			if (UnitSubsystem->FindNearestEnemyEntity(
				EntityLocation, 
				Target,
				UnitList[EntityIdx].GetOwner(),
				StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
			{
				AttackList[EntityIdx].LastAttackTime = 0.0;
				AttackList[EntityIdx].bIsTargetSet = true;
				AttackList[EntityIdx].TargetEntity = Target;
				
			}else
			{
				AttackList[EntityIdx].LastAttackTime = TimeSeconds;
			}
			
		}
	});
}
