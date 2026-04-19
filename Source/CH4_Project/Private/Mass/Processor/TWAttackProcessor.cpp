// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWAttackProcessor.h"

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
#include "Runtime/Engine/Internal/Kismet/BlueprintTypeConversions.h"

UTWAttackProcessor::UTWAttackProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server | (int32)EProcessorExecutionFlags::Standalone;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::SyncWorldToMass);

	bRequiresGameThreadExecution = true;
}

void UTWAttackProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTWCommandFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadWrite);

	EntityQuery.AddTagRequirement<FTWMassAttackingTag>(EMassFragmentPresence::All);

	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);

	ProcessorRequirements.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UTWAttackProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [&EntityManager](FMassExecutionContext& Context)
	{
		constexpr float SearchingInterval = 0.1f;
		if (Context.DoesArchetypeHaveTag<FTWMassDeadTag>())
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

		double TimeSeconds = Context.GetWorld()->GetTimeSeconds();

		TSet<FMassEntityHandle> EntitiesToSignalAttackComplete;

		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			if (false == AttackList[EntityIdx].bIsTargetSet)
			{
				continue;
			}
			FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
			FMassEntityHandle TargetEntity = AttackList[EntityIdx].TargetEntity;

			if (!EntityManager.IsEntityValid(TargetEntity))
			{
				AttackList[EntityIdx].bIsTargetSet = false;
				EntitiesToSignalAttackComplete.Add(Entity);
				continue;
			}

			FTWStatusFragment* EnemyStatusFragment = EntityManager.GetFragmentDataPtr<FTWStatusFragment>(TargetEntity);
			if (!EnemyStatusFragment || EnemyStatusFragment->GetIsDeath())
			{
				AttackList[EntityIdx].bIsTargetSet = false;
				EntitiesToSignalAttackComplete.Add(Entity);

				continue;
			}
			FTWUnitStatus& EnemyStatus = EnemyStatusFragment->GetMutableStatus();

			FVector EntityLocation = TransformList[EntityIdx].GetTransform().GetLocation();
			FVector EnemyLocation = EntityManager.GetFragmentDataPtr<FTransformFragment>(TargetEntity)->GetTransform().
			                                      GetLocation();

			if (FVector::DistSquared(EntityLocation, EnemyLocation) >
				FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
			{
				UE_LOG(LogTemp, Warning, TEXT("Entity %d Lose Target! "), EntityIdx)
				if (CommandList[EntityIdx].GetType() != ETWMassCommand::AttackToUnit)
				{
					AttackList[EntityIdx].bIsTargetSet = false;
					Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
				}
				if (CommandList[EntityIdx].GetType() == ETWMassCommand::AttackToUnit ||
					CommandList[EntityIdx].GetType() == ETWMassCommand::AttackToLocation)
				{
					SignalSubsystem->SignalEntity(PathInitSignal, Entity);
				}
				if (CommandList[EntityIdx].GetType() != ETWMassCommand::Hold &&
					CommandList[EntityIdx].GetType() != ETWMassCommand::None
				)
				{
					Context.Defer().AddTag<FTWMassMovingTag>(Entity);
				}
				continue;
			}
			//TODO Divide Optimization
			if (TimeSeconds - AttackList[EntityIdx].LastAttackTime < (1 / StatusList[EntityIdx].GetStatus().GetStatus(
				ETWStatusType::AttackSpeed)))
			{
				continue;
			}

			UE_LOG(LogTemp, Warning, TEXT("Entity %d Is Attack! "), EntityIdx)


			UE_LOG(LogTemp, Warning, TEXT("Before Target Health : %f  "),
			       EnemyStatus.Status[static_cast<int32>(ETWStatusType::Health)])
			
			TransformList[EntityIdx].GetMutableTransform().SetRotation(UKismetMathLibrary::FindLookAtRotation(EntityLocation, EnemyLocation).Quaternion());
			
			
			EnemyStatus.Status[static_cast<int32>(ETWStatusType::Health)] -= StatusList[EntityIdx].GetStatus().
				GetStatus(ETWStatusType::Damage);
			AttackList[EntityIdx].LastAttackTime = TimeSeconds;

			if (StatusList[EntityIdx].GetIsDeath())
			{
				AttackList[EntityIdx].bIsTargetSet = false;
				EntitiesToSignalAttackComplete.Add(Entity);
				UE_LOG(LogTemp, Warning, TEXT("Target Entity Dead!"))
			}
			UE_LOG(LogTemp, Warning, TEXT("After Target Health : %f  "),
			       EnemyStatus.Status[static_cast<int32>(ETWStatusType::Health)])
		}

		if (EntitiesToSignalAttackComplete.Num())
		{
			SignalSubsystem->SignalEntities(AttackCompleteSignal, EntitiesToSignalAttackComplete.Array());
		}
	});
}
