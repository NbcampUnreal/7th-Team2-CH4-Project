// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWDespawnProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "MassSignalSubsystem.h"
#include "Log/TWLogCategory.h"
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
		UTWUnitSubsystem* UnitSubsystem = Context.GetWorld()->GetSubsystem<UTWUnitSubsystem>();
		if (false == IsValid(UnitSubsystem))
		{
			return;
		}

		const TArrayView<FTWStatusFragment> StatusList = Context.GetMutableFragmentView<FTWStatusFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		double TimeSeconds = Context.GetWorld()->GetTimeSeconds();
		TArray<FMassEntityHandle> EntitiesToDestroy;

		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			FMassEntityHandle Entity = Context.GetEntity(EntityIdx);

			if (StatusList[EntityIdx].GetIsDeath()  &&
				StatusList[EntityIdx].GetDeathAnimationPlayed() == false)
			{
				StatusList[EntityIdx].SetDeathAnimationPlayed(true);
				if (ATWUnit* Unit = Cast<ATWUnit> (ActorList[EntityIdx].GetMutable()))
				{
					Unit->PlayDeathMontage();
				}
			}

			if (Context.GetWorld()->GetAuthGameMode() &&
				StatusList[EntityIdx].GetStatus().Status[static_cast<int32>(ETWStatusType::Health)] <= 0.0f &&
				StatusList[EntityIdx].GetIsDeath() == false
			)
			{
				UnitSubsystem->OnUnitKilled(Entity);
				StatusList[EntityIdx].GetMutableStatus().Status[static_cast<int32>(ETWStatusType::Health)] = 0.0f;
				StatusList[EntityIdx].SetDestroyTime(TimeSeconds + 3.0f);
				StatusList[EntityIdx].SetIsDeath(true);
				EntityManager.Defer().AddTag<FTWMassDeadTag>(Entity);

				UE_LOG(LogTWEntity, Warning, TEXT("Entity Dead!"))
			}


			if (StatusList[EntityIdx].GetDestroyTime() < TimeSeconds)
			{
				EntitiesToDestroy.Add(Entity);
				//Context.Defer().RemoveTag<FTWMassDeadTag>(Entity);
			}
		}
		Context.Defer().DestroyEntities(EntitiesToDestroy);
	});
}
