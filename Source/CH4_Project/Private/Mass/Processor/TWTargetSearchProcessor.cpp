// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWTargetSearchProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassNavigationTypes.h"
#include "MassNavMeshNavigationUtils.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "MassNavMeshNavigationFragments.h"

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
	EntityQuery.AddRequirement<FMassNavMeshShortPathFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>();
	EntityQuery.AddTagRequirement<FTWMassSearchingTag>(EMassFragmentPresence::All);
}

void UTWTargetSearchProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		constexpr float SearchingInterval = 0.1f;

		if (Context.DoesArchetypeHaveTag<FTWMassDeadTag>())
		{
			return;
		}
		
		UTWUnitSubsystem* UnitSubsystem = Context.GetWorld()->GetSubsystem<UTWUnitSubsystem>();
		if (false == IsValid(UnitSubsystem))
		{
			return;
		}

		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FTWStatusFragment> StatusList = Context.GetFragmentView<FTWStatusFragment>();
		const TConstArrayView<FTWUnitFragment> UnitList = Context.GetFragmentView<FTWUnitFragment>();
		const TArrayView<FTWAttackFragment> AttackList = Context.GetMutableFragmentView<FTWAttackFragment>();
		const TArrayView<FMassMoveTargetFragment> MoveTargetList = Context.GetMutableFragmentView<FMassMoveTargetFragment>();
		const TArrayView<FMassNavMeshShortPathFragment> NavMeshShortPathFragments = 
			Context.GetMutableFragmentView<FMassNavMeshShortPathFragment>();
		const FMassMovementParameters& MovementParameters = Context.GetConstSharedFragment<FMassMovementParameters>();

		double TimeSeconds = Context.GetWorld()->GetTimeSeconds();

		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			if (TimeSeconds - AttackList[EntityIdx].LastSearchTime < SearchingInterval)
			{
				continue;
			}
			FVector EntityLocation = TransformList[EntityIdx].GetTransform().GetLocation();
			FMassEntityHandle Target;
			ATWBaseBuilding* TargetBuilding = nullptr;
			if (UnitSubsystem->FindNearestEnemyEntity(
				EntityLocation,
				Target,
				UnitList[EntityIdx].GetOwner(),
				StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range))
				&&Context.GetEntityManagerChecked().IsEntityValid(Target)
				&&!Context.GetEntityManagerChecked().GetFragmentDataPtr<FTWStatusFragment>(Target)->GetIsDeath()
				)
			{
				FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
				AttackList[EntityIdx].bIsTargetSet = true;
				AttackList[EntityIdx].TargetEntity = Target;
				Context.Defer().AddTag<FTWMassAttackingTag>(Entity);
				Context.Defer().RemoveTag<FTWMassSearchingTag>(Entity);
				Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
#pragma region Stand
				UWorld* World = Context.GetWorld();
				MoveTargetList[EntityIdx].Center = EntityLocation;
				MoveTargetList[EntityIdx].CreateNewAction(EMassMovementAction::Stand, *World);

				UE::MassNavigation::ActivateActionStand(
					World,
					Entity,
					MovementParameters.DefaultDesiredSpeed,
					MoveTargetList[EntityIdx],
					NavMeshShortPathFragments[EntityIdx]);
#pragma endregion
			}else if (UnitSubsystem->FindNearestEnemyBuilding(
				EntityLocation, 
				TargetBuilding,
				UnitList[EntityIdx].GetOwner(),
				StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
			{
				FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
				AttackList[EntityIdx].bIsTargetSet = true;
				AttackList[EntityIdx].TargetBuilding = TargetBuilding;
				Context.Defer().AddTag<FTWMassAttackingTag>(Entity);
				Context.Defer().RemoveTag<FTWMassSearchingTag>(Entity);
				Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
#pragma region Stand
				UWorld* World = Context.GetWorld();
				MoveTargetList[EntityIdx].Center = EntityLocation;
				MoveTargetList[EntityIdx].CreateNewAction(EMassMovementAction::Stand, *World);

				UE::MassNavigation::ActivateActionStand(
					World,
					Entity,
					MovementParameters.DefaultDesiredSpeed,
					MoveTargetList[EntityIdx],
					NavMeshShortPathFragments[EntityIdx]);
#pragma endregion
			}
			else
			{
				AttackList[EntityIdx].LastSearchTime = TimeSeconds;
			}
		}
	});
}
