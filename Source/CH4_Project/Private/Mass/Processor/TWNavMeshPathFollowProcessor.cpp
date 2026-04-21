// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processor/TWNavMeshPathFollowProcessor.h"

#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavMeshNavigationFragments.h"
#include "NavigationSystem.h"
#include "NavCorridor.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/StateTree/TWStateTreeCommandTask.h"
#include "Mass/Traits/TWCommandTrait.h"


UTWNavMeshPathFollowProcessor::UTWNavMeshPathFollowProcessor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), EntityQuery(*this)
{
	bRequiresGameThreadExecution = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server | (int32)EProcessorExecutionFlags::Standalone;

	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	// ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
	//TODO Client Tasks

	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::SyncWorldToMass);
	
}

void UTWNavMeshPathFollowProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTWCommandFragment>(EMassFragmentAccess::ReadWrite);
	
	
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAgentRadiusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>();
	EntityQuery.AddRequirement<FMassNavMeshShortPathFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassNavMeshCachedPathFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);

	ProcessorRequirements.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
	
	// EntityQuery.AddTagRequirement<FTWMassMovingTag>(EMassFragmentPresence::All);
}

void UTWNavMeshPathFollowProcessor::SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context,
                                                   FMassSignalNameLookup& EntitySignals)
{
	
	
	auto TickFunc = [EntitySignals, &EntityManager, this](FMassExecutionContext& Context)
	{
		
		if (Context.DoesArchetypeHaveTag<FTWMassDeadTag>())
		{
			return;
		}
		UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(Context.GetWorld());
		if (false == IsValid(SignalSubsystem))
		{
			return;
		}	
		const TArrayView<FTWCommandFragment> CommandFragments = Context.GetMutableFragmentView<FTWCommandFragment>();
		const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassMoveTargetFragment> MoveTargetFragments = 
			Context.GetMutableFragmentView<FMassMoveTargetFragment>();
		const TConstArrayView<FAgentRadiusFragment> RadiusFragments = Context.GetFragmentView<FAgentRadiusFragment>();
		const TConstArrayView<FMassDesiredMovementFragment> DesiredMovementFragments = Context.GetFragmentView<FMassDesiredMovementFragment>();
		const FMassMovementParameters& MovementParameters = Context.GetConstSharedFragment<FMassMovementParameters>();
		const TArrayView<FMassNavMeshShortPathFragment> NavMeshShortPathFragments =
			Context.GetMutableFragmentView<FMassNavMeshShortPathFragment>();
		const TArrayView<FMassNavMeshCachedPathFragment> NavMeshCachedPathFragments =
			Context.GetMutableFragmentView<FMassNavMeshCachedPathFragment>();
		
		TArray<FMassEntityHandle> EntitiesToSignalPathDone;
		
		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
			TArray<FName> Signals;
			EntitySignals.GetSignalsForEntity(Entity, Signals);
			
			//MassNavMeshPathFollowTask::Tick
			if(Signals.Contains(UE::Mass::Signals::FollowPointPathDone))
			{
				FMassNavMeshShortPathFragment& ShortPathFragment = NavMeshShortPathFragments[EntityIdx];
			
				// Current path follow is done, but it was partial (i.e. many points on a curve), try again until we get there.
				if (ShortPathFragment.IsDone() && ShortPathFragment.bPartialResult)
				{
				
					FMassNavMeshCachedPathFragment& CachedPathFragment = NavMeshCachedPathFragments[EntityIdx];

					ShortPathFragment.RequestShortPath(CachedPathFragment.Corridor, CachedPathFragment.NavPathNextStartIndex, CachedPathFragment.NumLeadingPoints, 20.0f);

					CachedPathFragment.NavPathNextStartIndex += (uint16)FMath::Max(ShortPathFragment.NumPoints - FMassNavMeshShortPathFragment::NumPointsBeyondUpdate - FMassNavMeshCachedPathFragment::NumLeadingPoints, 0);

				}
	
				if (ShortPathFragment.IsDone())
				{
					EntitiesToSignalPathDone.Add(Entity);
				}
			}
			//MassNavMeshPathFollowTask::EnterState
			if (Signals.Contains(PathInitSignal))
			{
				if (!RequestPath(
					CommandFragments[EntityIdx].GetMoveDestination(),
					Context.GetWorld(),
					RadiusFragments[EntityIdx],
					TransformFragments[EntityIdx].GetTransform().GetLocation(),
					NavMeshCachedPathFragments[EntityIdx],
					NavMeshShortPathFragments[EntityIdx],
					MoveTargetFragments[EntityIdx],
					MovementParameters,
					DesiredMovementFragments[EntityIdx]
					))
				{
					CommandFragments[EntityIdx].SetType(ETWMassCommand::Hold);
					SignalSubsystem->SignalEntities(CommandSignal, {Entity});
					continue;
				}
			}
		}
		
		if (EntitiesToSignalPathDone.Num())
		{
			SignalSubsystem->SignalEntities(MoveCompleteSignal, EntitiesToSignalPathDone);
		}
	};

	EntityQuery.ForEachEntityChunk(Context, TickFunc);
}

void UTWNavMeshPathFollowProcessor::InitializeInternal(UObject& Owner,
                                                       const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);
	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(Owner.GetWorld());
	SubscribeToSignal(*SignalSubsystem, UE::Mass::Signals::FollowPointPathDone);
	SubscribeToSignal(*SignalSubsystem, PathInitSignal);
}



bool UTWNavMeshPathFollowProcessor::RequestPath(
	const FVector& TargetLocation, 
	const UWorld* World,
	const FAgentRadiusFragment& AgentRadius,
	const FVector AgentNavLocation,
	FMassNavMeshCachedPathFragment& CachedPathFragment,
	FMassNavMeshShortPathFragment& ShortPathFragment,
	FMassMoveTargetFragment& MoveTarget,
	const FMassMovementParameters& MovementParams,
	const FMassDesiredMovementFragment& DesiredMovementFragment
) const
{
//	FMassNavMeshPathFollowTaskInstanceData& InstanceData = Context.GetInstanceData<FMassNavMeshPathFollowTaskInstanceData>(*this);

	const FNavAgentProperties& NavAgentProperties = FNavAgentProperties(AgentRadius.Radius);

	const UNavigationSystemV1* NavMeshSubsystem = Cast<UNavigationSystemV1>(World->GetNavigationSystem());
	const ANavigationData* NavData = NavMeshSubsystem->GetNavDataForProps(NavAgentProperties, AgentNavLocation);
	
	if (!NavData)
	{
		return false;
	}
		
	FNavLocation ProjectedLocation;
	FVector NavTargetLocation = FVector::ZeroVector;
	if (NavMeshSubsystem && NavMeshSubsystem->ProjectPointToNavigation(TargetLocation, ProjectedLocation, FVector(500.f, 500.f, 500.f)))
	{
		NavTargetLocation = ProjectedLocation.Location;
	}
	
	
	FPathFindingQuery Query(NavMeshSubsystem, *NavData, AgentNavLocation, NavTargetLocation);

	// Why fix it after if there is none??
	if (!Query.NavData.IsValid())
	{
		Query.NavData = NavMeshSubsystem->GetNavDataForProps(NavAgentProperties, Query.StartLocation);
	}

	FPathFindingResult Result(ENavigationQueryResult::Error);
	if (Query.NavData.IsValid())
	{
		
		Result = Query.NavData->FindPath(NavAgentProperties, Query);
	}

	if (Result.IsSuccessful())
	{
		// @todo: Investigate single point paths but for now, only move if we have more than one point.
		if (Result.Path.Get()->GetPathPoints().Num() > 1)
		{
			CachedPathFragment.NavPath = Result.Path;

			// Build corridor
			CachedPathFragment.Corridor = MakeShared<FNavCorridor>();
			const FSharedConstNavQueryFilter NavQueryFilter = Query.QueryFilter ? Query.QueryFilter : NavData->GetDefaultQueryFilter();

			FNavCorridorParams CorridorParams;
			CorridorParams.SetFromWidth(300.0f);
			CorridorParams.PathOffsetFromBoundaries = 10.0f;

			CachedPathFragment.Corridor->BuildFromPath(*CachedPathFragment.NavPath, NavQueryFilter, CorridorParams);

			// Update short path

			ShortPathFragment.RequestShortPath(CachedPathFragment.Corridor, /*NavCorridorStartIndex*/0, /*NumLeadingPoints*/0, 20.0f);

			CachedPathFragment.NavPathNextStartIndex = (uint16)FMath::Max(ShortPathFragment.NumPoints - FMassNavMeshShortPathFragment::NumPointsBeyondUpdate - FMassNavMeshCachedPathFragment::NumLeadingPoints, 0);
			
			// Update MoveTarget
			float DesiredSpeed = MovementParams.MaxSpeed;

			// Apply DesiredMaxSpeedOverride
			DesiredSpeed = FMath::Min(DesiredSpeed, DesiredMovementFragment.DesiredMaxSpeedOverride);

			MoveTarget.DesiredSpeed.Set(DesiredSpeed);
			
			MoveTarget.CreateNewAction(EMassMovementAction::Move, *World);

			return true;
		}
	
		return true;
	}
	
	return false;
}