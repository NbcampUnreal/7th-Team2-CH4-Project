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
#include "Subsystems/TWUnitSubsystem.h"
#include "Core/TWPlayerState.h"
#include "Core/TWPlayerController.h"
#include "EngineUtils.h"
#include "MassNavigationUtils.h"
#include "MassReplicationFragments.h"
#include "Log/TWLogCategory.h"
#include "Runtime/Engine/Internal/Kismet/BlueprintTypeConversions.h"

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

		const double TimeSeconds = Context.GetWorld()->GetTimeSeconds();

		TSet<FMassEntityHandle> EntitiesToSignalAttackComplete;

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
			bool bIsBuildingValid = IsValid(TargetBuilding);
			
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

				//사거리 이탈
				if (FVector::DistSquared(EntityLocation, EnemyLocation) >
					FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
				{
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					
					if (CommandList[EntityIdx].GetType() == ETWMassCommand::Hold)
					{
						AttackList[EntityIdx].ClearTarget();
						Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
					}else 
					{
						Context.Defer().AddTag<FTWMassMovingTag>(Entity);
						Context.Defer().AddTag<FTWMassChasingTag>(Entity);
					}
					continue;
				}

				// TODO Divide Optimization
				if (TimeSeconds - AttackList[EntityIdx].LastAttackTime <
					(1 / StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::AttackSpeed)))
				{
					continue;
				}

				UE_LOG(LogTWEntity, Warning, TEXT("Entity %d Is Attack!"), EntityIdx);

				const int32 HealthIndex = static_cast<int32>(ETWStatusType::Health);
				const float OldHealth = EnemyStatus.Status[HealthIndex];
				const float DamageAmount = StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Damage);

				UE_LOG(LogTWEntity, Warning, TEXT("Before Target Health : %f"), OldHealth);

				const float NewHealth = FMath::Max(0.0f, OldHealth - DamageAmount);
				EnemyStatus.Status[HealthIndex] = NewHealth;
				AttackList[EntityIdx].LastAttackTime = TimeSeconds;
				
				
				const FVector::FReal NewHeading = UE::MassNavigation::GetYawFromDirection(EnemyLocation-EntityLocation);
				FQuat Rotation(FVector::UpVector, NewHeading);
				
				TransformList[EntityIdx].GetMutableTransform().SetRotation(Rotation);

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


				UE_LOG(LogTWEntity, Warning, TEXT("After Target Health : %f"), EnemyStatus.Status[HealthIndex]);
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
				const FVector EnemyLocation = TargetBuilding->GetTransform().GetLocation();

				if (FVector::DistSquared(EntityLocation, EnemyLocation) >
					FMath::Square(StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Range)))
				{
					Context.Defer().RemoveTag<FTWMassAttackingTag>(Entity);
					
					if (CommandList[EntityIdx].GetType() == ETWMassCommand::Hold)
					{
						AttackList[EntityIdx].ClearTarget();
						Context.Defer().AddTag<FTWMassSearchingTag>(Entity);
						
					}else if (CommandList[EntityIdx].GetType() == ETWMassCommand::AttackToLocation)
					{
						Context.Defer().AddTag<FTWMassMovingTag>(Entity);
						Context.Defer().AddTag<FTWMassChasingTag>(Entity);
					}else if (CommandList[EntityIdx].GetType() == ETWMassCommand::AttackToTarget)
					{
						Context.Defer().AddTag<FTWMassMovingTag>(Entity);
						Context.Defer().AddTag<FTWMassChasingTag>(Entity);
					}
					continue;
				}

				// TODO Divide Optimization
				if (TimeSeconds - AttackList[EntityIdx].LastAttackTime <
					(1 / StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::AttackSpeed)))
				{
					continue;
				}

				UE_LOG(LogTWEntity, Warning, TEXT("Entity %d Is Attack!"), EntityIdx);

				const float DamageAmount = StatusList[EntityIdx].GetStatus().GetStatus(ETWStatusType::Damage);

				UE_LOG(LogTWEntity, Warning, TEXT("Before Target Health : %f"), TargetBuilding->GetCurrentHP());
				
				TargetBuilding->ApplyDamageToBuilding(DamageAmount);

				AttackList[EntityIdx].LastAttackTime = TimeSeconds;
				TransformList[EntityIdx].GetMutableTransform().SetRotation(
					UKismetMathLibrary::FindLookAtRotation(EntityLocation, EnemyLocation).Quaternion());
				

				UE_LOG(LogTWEntity, Warning, TEXT("After Target Health : %f"), TargetBuilding->GetCurrentHP());
			}
		}

		if (EntitiesToSignalAttackComplete.Num())
		{
			SignalSubsystem->SignalEntities(AttackCompleteSignal, EntitiesToSignalAttackComplete.Array());
		}
	});
}
