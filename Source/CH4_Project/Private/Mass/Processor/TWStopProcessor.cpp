// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processor/TWStopProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "MassNavigationFragments.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"

UTWStopProcessor::UTWStopProcessor()
	:EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Avoidance);
	bRequiresGameThreadExecution = true;
}

void UTWStopProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWTransformOffsetFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWCommandTypeFragment>(EMassFragmentAccess::ReadOnly);
}

void UTWStopProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FTWTransformOffsetFragment> MeshOffsetList = Context.GetFragmentView<FTWTransformOffsetFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		
		const TConstArrayView<FTWCommandTypeFragment> CommandList = Context.GetFragmentView<FTWCommandTypeFragment>();
		const TArrayView<FMassMoveTargetFragment> MoveTargetList = Context.GetMutableFragmentView<FMassMoveTargetFragment>();
		
		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			AActor* Actor = ActorList[EntityIdx].GetMutable();
			if (!Actor) continue;
			
			const ETWState CurrentState = CommandList[EntityIdx].GetType();
			FTransform Transform;
			
			if (CurrentState == ETWState::Hold)
			{
				FMassMoveTargetFragment& MoveTarget = MoveTargetList[EntityIdx];
				MoveTarget.Center = TransformList[EntityIdx].GetTransform().GetLocation();
				MoveTarget.DesiredSpeed = FMassInt16Real(0.0f);
				MoveTarget.DistanceToGoal = 0.0f;
				MoveTarget.IntentAtGoal = EMassMovementAction::Stand;
				
				Actor->SetActorTransform(Actor->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
			}
			else
			{
				const FTransformFragment& TransformFragment = TransformList[EntityIdx];
			
				float YawOffset = 0.0f;
				if (MeshOffsetList.Num() > 0)
				{
					YawOffset = MeshOffsetList[EntityIdx].TransformOffset.GetYaw();
				}
				
				float ServerYawRadian = FMath::DegreesToRadians(Transform.Rotator().Yaw);
				FQuat FinalVisualQuat = FQuat(FVector::UpVector, ServerYawRadian + FMath::DegreesToRadians(YawOffset));
				
				Transform.SetRotation(FinalVisualQuat);
				
				Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::None);
			}
		}
	});
}
