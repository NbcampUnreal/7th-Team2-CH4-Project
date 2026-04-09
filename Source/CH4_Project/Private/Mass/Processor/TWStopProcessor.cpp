// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processor/TWStopProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Traits/FWStandTrait.h"

UTWStopProcessor::UTWStopProcessor()
	:EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
	RequiredTags.Add<FFWStopMovementTag>();
	bRequiresGameThreadExecution = true;
}

void UTWStopProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	AddRequiredTagsToQuery(EntityQuery);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWTransformOffsetFragment>(EMassFragmentAccess::ReadWrite);
}

void UTWStopProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
	const TConstArrayView<FTWTransformOffsetFragment> MeshOffsetList = Context.GetFragmentView<FTWTransformOffsetFragment>();
	const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
	
	for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
	{
		const FTransformFragment& TransformFragment = TransformList[EntityIdx];
		
		FTransform Transform = TransformFragment.GetTransform();
		
		float YawOffset = 0.0f;
		if (MeshOffsetList.Num() > 0)
		{
			YawOffset = MeshOffsetList[EntityIdx].TransformOffset.GetYaw();
		}
		
		float ServerYawRadian = FMath::DegreesToRadians(Transform.Rotator().Yaw);
		FQuat ServerQuat = FQuat(FVector::UpVector, ServerYawRadian);
		
		FQuat OffsetQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(YawOffset));
		
		FQuat FinalVisualQuat = OffsetQuat * ServerQuat;
		
		Transform.SetRotation(FinalVisualQuat);
		
		if (AActor* Actor = ActorList[EntityIdx].GetMutable())
		{
			Actor->SetActorTransform(Transform, false, nullptr,
				ETeleportType::TeleportPhysics);
		}
	}
}
