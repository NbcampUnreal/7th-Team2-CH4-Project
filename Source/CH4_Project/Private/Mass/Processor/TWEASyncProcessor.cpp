// Fill out your copyright notice in the Description page of Project Settings.
#include "Mass/Processor/TWEASyncProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "GameFramework/Actor.h"
#include "MassActorSubsystem.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Traits/TWCommandTrait.h"
#include "Mass/Traits/TWEASyncTrait.h"

UTWEASyncProcessor::UTWEASyncProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	RequiredTags.Add<FTWEASyncTag>();
	bRequiresGameThreadExecution = true;
}

void UTWEASyncProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	AddRequiredTagsToQuery(EntityQuery);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTWTransformOffsetFragment>(EMassFragmentAccess::ReadWrite);
}

void UTWEASyncProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// EntityQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& Context)
	// {	
	// 	const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
	// 	const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
	// 	const int32 NumEntities = Context.GetNumEntities();
	// 	for (int32 i = 0; i < NumEntities; ++i)
	// 	{
	// 		if (AActor* Actor = ActorList[i].GetMutable())
	// 		{
	// 			Actor->SetActorTransform(TransformList[i].GetTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	// 		}
	// 	}
	// });

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		if (Context.DoesArchetypeHaveTag<FTWMassDeadTag>())
		{
			return;
		}
		
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FTWTransformOffsetFragment> MeshOffsetList = Context.GetFragmentView<FTWTransformOffsetFragment>();
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
	
		for (int32 EntityIdx = 0; EntityIdx < Context.GetNumEntities(); EntityIdx++)
		{
			const FTransformFragment& TransformFragment = TransformList[EntityIdx];
	
			FVector TranslationOffset = FVector::ZeroVector;
			float YawOffset = 0.0f;
			if (MeshOffsetList.Num() > 0)
			{
				TranslationOffset = MeshOffsetList[EntityIdx].TransformOffset.GetPosition();
				YawOffset = MeshOffsetList[EntityIdx].TransformOffset.GetYaw();
			}
	
			FTransform Transform = TransformFragment.GetTransform();
			Transform.SetLocation(Transform.GetLocation() + TranslationOffset);
			

			
			float ServerYawRadian =  FMath::DegreesToRadians(Transform.Rotator().Yaw);
			FQuat ServerQuat = FQuat(FVector::UpVector, ServerYawRadian);
	
			float VisualYawOffsetDeg = YawOffset;
			FQuat OffsetQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(VisualYawOffsetDeg));
	
			FQuat FinalVisualQuat = OffsetQuat * ServerQuat;
	
			Transform.SetRotation(FinalVisualQuat);
	
			if (AActor* Actor = ActorList[EntityIdx].GetMutable())
			{
				Actor->SetActorTransform(Transform, false, nullptr,
				                         ETeleportType::TeleportPhysics);
			}
		}
	});
}
