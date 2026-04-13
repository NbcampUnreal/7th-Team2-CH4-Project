// Fill out your copyright notice in the Description page of Project Settings.



#include "Mass/Replication/BubbleHandler/TWTransformSmoothMassClientBubbleHandler.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Fragments/TWTransformOffsetParams.h"

#if UE_REPLICATION_COMPILE_CLIENT_CODE  
void UTWTransformSmoothMassClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView,
                                                                    const FTWTransformReplicatedAgent& Item)
{
	FTransformFragment& TransformFragment = EntityView.GetFragmentData<FTransformFragment>();
  	
	const FVector PreviousLocation = TransformFragment.GetTransform().GetLocation();
	const float PreviousYaw = FRotator::NormalizeAxis(TransformFragment.GetTransform().Rotator().Yaw);
	FTWTransformMassClientBubbleHandler::PostReplicatedChangeEntity(EntityView, Item);
	const FVector NewLocation = TransformFragment.GetTransform().GetLocation();
	const float NewYaw = FRotator::NormalizeAxis(TransformFragment.GetTransform().Rotator().Yaw);
  
 
	// Initializes mesh offset 
	FTWTransformOffsetFragment& TranslationOffset = EntityView.GetFragmentData<FTWTransformOffsetFragment>();
	const FTWTransformOffsetParams& OffsetParams = EntityView.GetConstSharedFragmentData<FTWTransformOffsetParams>();
  	
	if (OffsetParams.MaxSmoothNetUpdateDistanceSqr > FVector::DistSquared(PreviousLocation, NewLocation))
	{
		// Offsetting the mesh to sync with the sever locations smoothly
		TranslationOffset.TransformOffset.SetPosition( TranslationOffset.TransformOffset.GetPosition()+ PreviousLocation - NewLocation);
	}
	
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(NewYaw, PreviousYaw);
	if (OffsetParams.MaxSmoothNetUpdateYaw > FMath::Abs(DeltaYaw))
	{
		float CurrentYawOffset = TranslationOffset.TransformOffset.GetYaw();
		TranslationOffset.TransformOffset.SetYaw(FRotator::NormalizeAxis(CurrentYawOffset + DeltaYaw));
	}
	
}
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE  
