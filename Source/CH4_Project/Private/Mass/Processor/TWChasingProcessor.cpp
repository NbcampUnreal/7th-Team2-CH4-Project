// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWChasingProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "MassSignalSubsystem.h"
#include "Quaternion.h"
#include "Kismet/KismetMathLibrary.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Mass/Traits/TWCommandTrait.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Core/TWPlayerState.h"
#include "Core/TWPlayerController.h"
#include "EngineUtils.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassNavMeshNavigationUtils.h"

#include "MassNavMeshNavigationFragments.h"
#include "NavigationSystem.h"
#include "NavCorridor.h"

#include "MassReplicationFragments.h"
#include "Runtime/Engine/Internal/Kismet/BlueprintTypeConversions.h"


UTWChasingProcessor::UTWChasingProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server | (int32)EProcessorExecutionFlags::Standalone;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::SyncWorldToMass);

	bRequiresGameThreadExecution = true;
}

void UTWChasingProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTWCommandFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassNavMeshShortPathFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FMassMovementParameters>();
	EntityQuery.AddTagRequirement<FTWMassChasingTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FTWMassMovingTag>(EMassFragmentPresence::All);

	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);

	ProcessorRequirements.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UTWChasingProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [&EntityManager](FMassExecutionContext& Context)
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

		UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(Context.GetWorld());
		if (false == IsValid(SignalSubsystem))
		{
			return;
		}

		const TArrayView<FTWCommandFragment> CommandList = Context.GetMutableFragmentView<FTWCommandFragment>();
		const TArrayView<FTransformFragment> TransformList = Context.GetMutableFragmentView<FTransformFragment>();
		const TConstArrayView<FTWStatusFragment> StatusList = Context.GetFragmentView<FTWStatusFragment>();
		const TArrayView<FTWAttackFragment> AttackList = Context.GetMutableFragmentView<FTWAttackFragment>();
		const TArrayView<FMassMoveTargetFragment> MoveTargetList = Context.GetMutableFragmentView<
			FMassMoveTargetFragment>();
		const TArrayView<FMassNavMeshShortPathFragment> NavMeshShortPathFragments =
			Context.GetMutableFragmentView<FMassNavMeshShortPathFragment>();
		const FMassMovementParameters& MovementParameters = Context.GetConstSharedFragment<FMassMovementParameters>();
		const double TimeSeconds = Context.GetWorld()->GetTimeSeconds();

		TSet<FMassEntityHandle> EntitiesToSignalAttackComplete;
		TSet<FMassEntityHandle> EntitiesToSignalPathInit;
		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			const FMassEntityHandle Entity = Context.GetEntity(EntityIdx);	
			
			const FMassEntityHandle TargetEntity = AttackList[EntityIdx].GetTargetEntity();
			ATWBaseBuilding* TargetBuilding = AttackList[EntityIdx].GetTargetBuilding();
			if (false == AttackList[EntityIdx].GetIsTargetSet())
			{
				EntitiesToSignalAttackComplete.Add(Entity);
				continue;
			}

			bool bIsEntityValid = EntityManager.IsEntityValid(TargetEntity);
			bool bIsBuildingValid = IsValid(AttackList[EntityIdx].GetTargetBuilding());

			if (!bIsEntityValid && !bIsBuildingValid)
			{
				AttackList[EntityIdx].ClearTarget();
				EntitiesToSignalAttackComplete.Add(Entity);
				continue;
			}


			if (bIsEntityValid)
			{
				FTWStatusFragment* EnemyStatusFragment = EntityManager.GetFragmentDataPtr<FTWStatusFragment>(
					TargetEntity);
				if (!EnemyStatusFragment || EnemyStatusFragment->GetIsDeath())
				{
					AttackList[EntityIdx].ClearTarget();
					EntitiesToSignalAttackComplete.Add(Entity);
					continue;
				}


				FTWUnitStatus& EnemyStatus = EnemyStatusFragment->GetMutableStatus();

				const FVector EntityLocation = TransformList[EntityIdx].GetTransform().GetLocation();

				const FTransformFragment* TargetTransformFragment =
					EntityManager.GetFragmentDataPtr<FTransformFragment>(TargetEntity);
				if (!TargetTransformFragment)
				{
					AttackList[EntityIdx].ClearTarget();
					EntitiesToSignalAttackComplete.Add(Entity);
					continue;
				}


				const FVector EnemyLocation = TargetTransformFragment->GetTransform().GetLocation();

				if (FVector::DistSquared(EntityLocation, EnemyLocation) <
					FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
				{
					Context.Defer().AddTag<FTWMassAttackingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
#pragma region Stand
					UWorld* World = Context.GetWorld();
					CommandList[EntityIdx].SetMoveDestination(EntityLocation);
					MoveTargetList[EntityIdx].CreateNewAction(EMassMovementAction::Stand, *World);

					UE::MassNavigation::ActivateActionStand(
						World,
						Entity,
						MovementParameters.DefaultDesiredSpeed,
						MoveTargetList[EntityIdx],
						NavMeshShortPathFragments[EntityIdx]);
#pragma endregion
					 
					continue;
				}

				if (TimeSeconds < AttackList[EntityIdx].LastChasingTime + 0.1f)
				{
					continue;
				}
				AttackList[EntityIdx].LastChasingTime = TimeSeconds;

				//TODO NavigationInit
				if (CommandList[EntityIdx].GetType()==ETWMassCommand::AttackToTarget||
					FVector::DistSquared(EntityLocation, EnemyLocation) <
					FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::SearchingRange)))
				{
					CommandList[EntityIdx].SetMoveDestination(EnemyLocation);
					EntitiesToSignalPathInit.Add(Entity);
				}else
				{
					EntitiesToSignalAttackComplete.Add(Entity);
				}
			}
			else if (bIsBuildingValid)
			{
				if (TargetBuilding->IsDead())
				{
					AttackList[EntityIdx].ClearTarget();
					EntitiesToSignalAttackComplete.Add(Entity);
					continue;
				}

				const FVector EntityLocation = TransformList[EntityIdx].GetTransform().GetLocation();

				
				const FVector BuildingLocation = TargetBuilding->GetTransform().GetLocation();

				if (FVector::DistSquared(EntityLocation, BuildingLocation) <
					FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
				{

					Context.Defer().AddTag<FTWMassAttackingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassChasingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassMovingTag>(Entity);
#pragma region Stand
					UWorld* World = Context.GetWorld();
					CommandList[EntityIdx].SetMoveDestination(EntityLocation);
					MoveTargetList[EntityIdx].CreateNewAction(EMassMovementAction::Stand, *World);

					UE::MassNavigation::ActivateActionStand(
						World,
						Entity,
						MovementParameters.DefaultDesiredSpeed,
						MoveTargetList[EntityIdx],
						NavMeshShortPathFragments[EntityIdx]);
#pragma endregion
					
					continue;
				}

				if (TimeSeconds < AttackList[EntityIdx].LastChasingTime + 0.1f)
				{
					continue;
				}
				AttackList[EntityIdx].LastChasingTime = TimeSeconds;

				//TODO NavigationInit
				if (CommandList[EntityIdx].GetType()==ETWMassCommand::AttackToTarget||
					FVector::DistSquared(EntityLocation, BuildingLocation) <
					FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::SearchingRange)))
				{
					CommandList[EntityIdx].SetMoveDestination(BuildingLocation);
					
					EntitiesToSignalPathInit.Add(Entity);
				}else
				{
					EntitiesToSignalAttackComplete.Add(Entity);
				}
				
			}
		}

		if (EntitiesToSignalAttackComplete.Num())
		{
			SignalSubsystem->SignalEntities(AttackCompleteSignal, EntitiesToSignalAttackComplete.Array());
		}
		if (EntitiesToSignalPathInit.Num())
		{
			SignalSubsystem->SignalEntities(PathInitSignal, EntitiesToSignalPathInit.Array());
		}
	});
}
