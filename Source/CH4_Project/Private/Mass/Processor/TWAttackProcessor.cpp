// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWAttackProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "MassSignalSubsystem.h"
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
#include "MassReplicationFragments.h"

namespace
{
	static bool TryGetTargetNetIdAndOwnerSlot(
		FMassEntityManager& EntityManager,
		const FMassEntityHandle& TargetEntity,
		FMassNetworkID& OutNetId,
		int32& OutOwnerPlayerSlot
	)
	{
		OutNetId = FMassNetworkID();
		OutOwnerPlayerSlot = INDEX_NONE;

		if (!EntityManager.IsEntityValid(TargetEntity))
		{
			return false;
		}

		const FMassNetworkIDFragment* NetIdFragment =
			EntityManager.GetFragmentDataPtr<FMassNetworkIDFragment>(TargetEntity);

		const FTWUnitFragment* UnitFragment =
			EntityManager.GetFragmentDataPtr<FTWUnitFragment>(TargetEntity);

		if (!NetIdFragment || !UnitFragment)
		{
			return false;
		}

		OutNetId = NetIdFragment->NetID;
		OutOwnerPlayerSlot = UnitFragment->GetOwner();
		return true;
	}

	static bool TryGetEntityOwnerSlot(
		FMassEntityManager& EntityManager,
		const FMassEntityHandle& Entity,
		int32& OutOwnerPlayerSlot
	)
	{
		OutOwnerPlayerSlot = INDEX_NONE;

		if (!EntityManager.IsEntityValid(Entity))
		{
			return false;
		}

		const FTWUnitFragment* UnitFragment =
			EntityManager.GetFragmentDataPtr<FTWUnitFragment>(Entity);

		if (!UnitFragment)
		{
			return false;
		}

		OutOwnerPlayerSlot = UnitFragment->GetOwner();
		return OutOwnerPlayerSlot != INDEX_NONE;
	}

	static void NotifyRelevantPlayerControllersRecentCombatDamage(
		UWorld* World,
		const FMassNetworkID& TargetNetId,
		int32 AttackerOwnerPlayerSlot,
		int32 TargetOwnerPlayerSlot,
		float VisibleTime = 1.25f
	)
	{
		if (!World || !TargetNetId.IsValid())
		{
			return;
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			ATWPlayerController* TWPC = Cast<ATWPlayerController>(It->Get());
			if (!TWPC)
			{
				continue;
			}

			const ATWPlayerState* TWPS = TWPC->GetPlayerState<ATWPlayerState>();
			if (!TWPS)
			{
				continue;
			}

			const int32 ControllerPlayerSlot = TWPS->PlayerSlot;

			const bool bIsRelevant =
				(ControllerPlayerSlot == AttackerOwnerPlayerSlot) ||
				(ControllerPlayerSlot == TargetOwnerPlayerSlot);

			if (!bIsRelevant)
			{
				continue;
			}

			TWPC->ClientNotifyRecentCombatUnitDamaged(
				TargetNetId,
				TargetOwnerPlayerSlot,
				VisibleTime
			);
		}
	}
}

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
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
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
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FTWStatusFragment> StatusList = Context.GetFragmentView<FTWStatusFragment>();
		const TArrayView<FTWAttackFragment> AttackList = Context.GetMutableFragmentView<FTWAttackFragment>();

		const double TimeSeconds = Context.GetWorld()->GetTimeSeconds();

		TArray<FMassEntityHandle> EntitiesToSignalAttackComplete;

		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			if (false == AttackList[EntityIdx].bIsTargetSet)
			{
				continue;
			}

			const FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
			const FMassEntityHandle TargetEntity = AttackList[EntityIdx].TargetEntity;

			if (!EntityManager.IsEntityValid(TargetEntity))
			{
				Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
				Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
				AttackList[EntityIdx].bIsTargetSet = false;
				CommandList[EntityIdx].SetType(ETWMassCommand::None);
				continue;
			}

			FTWStatusFragment* EnemyStatusFragment = EntityManager.GetFragmentDataPtr<FTWStatusFragment>(TargetEntity);
			if (!EnemyStatusFragment || EnemyStatusFragment->GetIsDeath())
			{
				Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
				Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
				AttackList[EntityIdx].bIsTargetSet = false;
				CommandList[EntityIdx].SetType(ETWMassCommand::None);
				continue;
			}

			FTWUnitStatus& EnemyStatus = EnemyStatusFragment->GetMutableStatus();

			const FVector EntityLocation = TransformList[EntityIdx].GetTransform().GetLocation();

			const FTransformFragment* TargetTransformFragment =
				EntityManager.GetFragmentDataPtr<FTransformFragment>(TargetEntity);
			if (!TargetTransformFragment)
			{
				Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
				Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
				AttackList[EntityIdx].bIsTargetSet = false;
				CommandList[EntityIdx].SetType(ETWMassCommand::None);
				continue;
			}

			const FVector EnemyLocation = TargetTransformFragment->GetTransform().GetLocation();

			if (FVector::DistSquared(EntityLocation, EnemyLocation) >
				FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
			{
				UE_LOG(LogTemp, Warning, TEXT("Entity %d Lose Target!"), EntityIdx);

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
					CommandList[EntityIdx].GetType() != ETWMassCommand::None)
				{
					Context.Defer().AddTag<FTWMassMovingTag>(Entity);
				}
				continue;
			}

			// TODO Divide Optimization
			if (TimeSeconds - AttackList[EntityIdx].LastAttackTime <
				(1 / StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::AttackSpeed)))
			{
				continue;
			}

			UE_LOG(LogTemp, Warning, TEXT("Entity %d Is Attack!"), EntityIdx);

			const int32 HealthIndex = static_cast<int32>(ETWStatusType::Health);
			const float OldHealth = EnemyStatus.Status[HealthIndex];
			const float DamageAmount = StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Damage);

			UE_LOG(LogTemp, Warning, TEXT("Before Target Health : %f"), OldHealth);

			const float NewHealth = FMath::Max(0.0f, OldHealth - DamageAmount);
			EnemyStatus.Status[HealthIndex] = NewHealth;
			AttackList[EntityIdx].LastAttackTime = TimeSeconds;

			if (NewHealth < OldHealth)
			{
				FMassNetworkID TargetNetId;
				int32 TargetOwnerPlayerSlot = INDEX_NONE;
				int32 AttackerOwnerPlayerSlot = INDEX_NONE;

				const bool bHasTargetInfo =
					TryGetTargetNetIdAndOwnerSlot(EntityManager, TargetEntity, TargetNetId, TargetOwnerPlayerSlot);

				const bool bHasAttackerOwner =
					TryGetEntityOwnerSlot(EntityManager, Entity, AttackerOwnerPlayerSlot);

				if (bHasTargetInfo)
				{
					NotifyRelevantPlayerControllersRecentCombatDamage(
						Context.GetWorld(),
						TargetNetId,
						bHasAttackerOwner ? AttackerOwnerPlayerSlot : INDEX_NONE,
						TargetOwnerPlayerSlot,
						1.25f
					);
				}
			}

			if (EnemyStatus.Status[HealthIndex] <= 0.0f)
			{
				FMassEntityHandle MutableTarget = TargetEntity;
				UnitSubsystem->OnUnitKilled(MutableTarget);
				EnemyStatus.Status[HealthIndex] = 0.0f;
				EnemyStatusFragment->SetDestroyTime(TimeSeconds + 3.0f);
				EnemyStatusFragment->SetIsDeath(true);
				EntityManager.Defer().AddTag<FTWMassDeadTag>(TargetEntity);

				AttackList[EntityIdx].bIsTargetSet = false;
				Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
				Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
				EntitiesToSignalAttackComplete.Add(Entity);

				UE_LOG(LogTemp, Warning, TEXT("Target Entity Dead!"));
			}

			UE_LOG(LogTemp, Warning, TEXT("After Target Health : %f"), EnemyStatus.Status[HealthIndex]);
		}

		if (EntitiesToSignalAttackComplete.Num())
		{
			SignalSubsystem->SignalEntities(AttackCompleteSignal, EntitiesToSignalAttackComplete);
		}
	});
}