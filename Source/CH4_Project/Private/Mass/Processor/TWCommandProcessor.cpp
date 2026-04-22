// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processor/TWCommandProcessor.h"

#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "MassNavigationTypes.h"
#include "MassNavMeshNavigationUtils.h"
#include "MassNavMeshNavigationFragments.h"
#include "MassMovementFragments.h"
#include "MassSignalSubsystem.h"
#include "Log/TWLogCategory.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Traits/TWCommandTrait.h"


UTWCommandProcessor::UTWCommandProcessor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), EntityQuery(*this)
{
	bRequiresGameThreadExecution = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server | (int32)EProcessorExecutionFlags::Standalone;

	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	// ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
	//TODO Client Tasks

	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::SyncWorldToMass);
}

void UTWCommandProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTWCommandFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassNavMeshShortPathFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>();

	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);

	ProcessorRequirements.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UTWCommandProcessor::SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context,
                                         FMassSignalNameLookup& EntitySignals)
{
	auto TickFunc = [&EntitySignals, &EntityManager](FMassExecutionContext& Context)
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
		const TArrayView<FMassMoveTargetFragment> MoveTargetFragments = Context.GetMutableFragmentView<
			FMassMoveTargetFragment>();
		const TArrayView<FTWAttackFragment> AttackFragments = Context.GetMutableFragmentView<
			FTWAttackFragment>();
		const TArrayView<FMassNavMeshShortPathFragment> NavMeshShortPathFragments = Context.GetMutableFragmentView<
			FMassNavMeshShortPathFragment>();
		const FMassMovementParameters& MovementParameters = Context.GetConstSharedFragment<FMassMovementParameters>();

		TArray<FMassEntityHandle> EntitiesToSignalPathInit;

		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
			TArray<FName> Signals;
			EntitySignals.GetSignalsForEntity(Entity, Signals);
			if (Signals.Contains(CommandSignal))
			{
				FMassNavMeshShortPathFragment& ShortPath = NavMeshShortPathFragments[EntityIdx];
				FMassMoveTargetFragment& MoveTarget = MoveTargetFragments[EntityIdx];

				const UWorld* World = Context.GetWorld();
				checkf(World != nullptr, TEXT("A valid world is expected from the execution context."));

				const FVector AgentNavLocation = TransformFragments[EntityIdx].GetTransform().GetLocation();
				
				//TODO Navigation Init
				switch (CommandFragments[EntityIdx].GetType())
				{
				case ETWMassCommand::None:
					check(false);
					break;
				case ETWMassCommand::MoveToLocation:
					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassSearchingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassHoldTag>(Entity);
					Context.Defer().AddTag<FTWMassMovingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					CommandFragments[EntityIdx].SetMoveDestination(CommandFragments[EntityIdx].GetLocation());
					EntitiesToSignalPathInit.Add(Entity);
					break;
				case ETWMassCommand::MoveToTarget:
					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassSearchingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassHoldTag>(Entity);
					Context.Defer().AddTag<FTWMassMovingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					CommandFragments[EntityIdx].SetMoveDestination(EntityManager.GetFragmentDataPtr<FTransformFragment>(CommandFragments[EntityIdx].GetTarget())->GetTransform().GetLocation());
					EntitiesToSignalPathInit.Add(Entity);
					break;
				case ETWMassCommand::AttackToLocation:
					{
						UE::Math::TVector<double> MyLocation = TransformFragments[EntityIdx].GetTransform().
							GetLocation();
						UE::Math::TVector<double> TargetLocation = CommandFragments[EntityIdx].GetLocation();
						UE_LOG(LogTWEntity, Warning, TEXT("%lf, %lf, %lf -> %lf, %lf, %lf"), MyLocation.X, MyLocation.Y,
						       MyLocation.Z, TargetLocation.X, TargetLocation.Y, TargetLocation.Z)
					}

					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassHoldTag>(Entity);
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					Context.Defer().AddTag<FTWMassMovingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					CommandFragments[EntityIdx].SetMoveDestination( CommandFragments[EntityIdx].GetLocation());
					EntitiesToSignalPathInit.Add(Entity);
					break;
				case ETWMassCommand::AttackToTarget:
					Context.Defer().RemoveTag<FTWMassSearchingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassHoldTag>(Entity);
					Context.Defer().AddTag<FTWMassMovingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					Context.Defer().AddTag<FTWMassChasingTag>(Entity);

					AttackFragments[EntityIdx].bIsTargetSet = true;
					AttackFragments[EntityIdx].TargetEntity = CommandFragments[EntityIdx].GetTarget();
					if (CommandFragments[EntityIdx].GetTarget().IsValid())
					{
						CommandFragments[EntityIdx].SetMoveDestination( EntityManager.GetFragmentDataPtr<FTransformFragment>(CommandFragments[EntityIdx].GetTarget())->GetTransform().GetLocation());
					}else if (CommandFragments[EntityIdx].GetTargetBuilding())
					{
						CommandFragments[EntityIdx].SetMoveDestination(CommandFragments[EntityIdx].GetTargetBuilding()->GetTransform().GetLocation() );
						
					}
					EntitiesToSignalPathInit.Add(Entity);
					break;
				case ETWMassCommand::Hold:
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassHoldTag>(Entity);
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
#pragma region Stand
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
					CommandFragments[EntityIdx].SetMoveDestination( AgentNavLocation);
					MoveTarget.CreateNewAction(EMassMovementAction::Stand, *World);

					UE::MassNavigation::ActivateActionStand(
						World,
						Entity,
						MovementParameters.DefaultDesiredSpeed,
						MoveTarget,
						ShortPath);
#pragma endregion
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
					//TODO
					break;
				}
			}
			if (Signals.Contains(MoveCompleteSignal))
			{
				FMassNavMeshShortPathFragment& ShortPath = NavMeshShortPathFragments[EntityIdx];
				FMassMoveTargetFragment& MoveTarget = MoveTargetFragments[EntityIdx];

				const UWorld* World = Context.GetWorld();
				checkf(World != nullptr, TEXT("A valid world is expected from the execution context."));

				const FVector AgentNavLocation = TransformFragments[EntityIdx].GetTransform().GetLocation();

				switch (CommandFragments[EntityIdx].GetType())
				{
				case ETWMassCommand::None:
					break;
				case ETWMassCommand::MoveToLocation:
#pragma region Stand
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
					CommandFragments[EntityIdx].SetMoveDestination( AgentNavLocation);
					MoveTarget.CreateNewAction(EMassMovementAction::Stand, *World);

					UE::MassNavigation::ActivateActionStand(
						World,
						Entity,
						MovementParameters.DefaultDesiredSpeed,
						MoveTarget,
						ShortPath);
#pragma endregion
					Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
					continue;
					break;
				case ETWMassCommand::MoveToTarget:
					{
						double DistSquared = 0.0f;
						if (CommandFragments[EntityIdx].GetTarget().IsValid())
						{
							DistSquared = FVector::DistSquared(
								TransformFragments[EntityIdx].GetTransform().GetLocation(),
								EntityManager.GetFragmentDataChecked<FTransformFragment>(
									CommandFragments[EntityIdx].GetTarget()).GetTransform().GetLocation()
							);
						}
						else
						{
							DistSquared = FVector::DistSquared(
								TransformFragments[EntityIdx].GetTransform().GetLocation(),
								CommandFragments[EntityIdx].GetTargetBuilding()->GetTransform().GetLocation()
							);
						}
						if (DistSquared < 10000.0f)
						{
#pragma region Stand
							CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
							CommandFragments[EntityIdx].SetMoveDestination( AgentNavLocation);

							MoveTarget.CreateNewAction(EMassMovementAction::Stand, *World);

							UE::MassNavigation::ActivateActionStand(
								World,
								Entity,
								MovementParameters.DefaultDesiredSpeed,
								MoveTarget,
								ShortPath
							);


							Context.Defer().RemoveTag<FTWMassHoldTag>(Entity);
							Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
							Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
							continue;
#pragma endregion
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
						}
						else
						{
							CommandFragments[EntityIdx].SetMoveDestination( EntityManager.GetFragmentDataPtr<FTransformFragment>(CommandFragments[EntityIdx].GetTarget())->GetTransform().GetLocation());
							EntitiesToSignalPathInit.Add(Entity);
						}
					}
					break;
				case ETWMassCommand::AttackToLocation:
#pragma region Stand
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
					CommandFragments[EntityIdx].SetMoveDestination(AgentNavLocation);

					MoveTarget.CreateNewAction(EMassMovementAction::Stand, *World);

					UE::MassNavigation::ActivateActionStand(
						World,
						Entity,
						MovementParameters.DefaultDesiredSpeed,
						MoveTarget,
						ShortPath);
#pragma endregion
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);

					Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					continue;
					break;
				case ETWMassCommand::AttackToTarget:
					CommandFragments[EntityIdx].SetMoveDestination(EntityManager.GetFragmentDataPtr<FTransformFragment>(CommandFragments[EntityIdx].GetTarget())->GetTransform().GetLocation());
					EntitiesToSignalPathInit.Add(Entity);
					break;
				case ETWMassCommand::Hold:

					break;
				}
			}
			if (Signals.Contains(AttackCompleteSignal))
			{
				switch (CommandFragments[EntityIdx].GetType())
				{
				case ETWMassCommand::None:
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					break;
				case ETWMassCommand::MoveToLocation:
					break;
				case ETWMassCommand::MoveToTarget:
					break;
				case ETWMassCommand::AttackToLocation:
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					Context.Defer().AddTag<FTWMassMovingTag>(Entity);
					CommandFragments[EntityIdx].SetMoveDestination(CommandFragments[EntityIdx].GetLocation());

					EntitiesToSignalPathInit.Add(Entity);
					break;
				case ETWMassCommand::AttackToTarget:
					CommandFragments[EntityIdx].SetType(ETWMassCommand::None);
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassHoldTag>(Entity);
					break;
				case ETWMassCommand::Hold:

					break;
				}
			}
		}
		if (EntitiesToSignalPathInit.Num())
		{
			SignalSubsystem->SignalEntities(PathInitSignal, EntitiesToSignalPathInit);
		}
	};

	EntityQuery.ForEachEntityChunk(Context, TickFunc);
}

void UTWCommandProcessor::InitializeInternal(UObject& Owner,
                                             const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);
	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(Owner.GetWorld());

	SubscribeToSignal(*SignalSubsystem, CommandSignal);
	SubscribeToSignal(*SignalSubsystem, AttackCompleteSignal);
	SubscribeToSignal(*SignalSubsystem, MoveCompleteSignal);
}
