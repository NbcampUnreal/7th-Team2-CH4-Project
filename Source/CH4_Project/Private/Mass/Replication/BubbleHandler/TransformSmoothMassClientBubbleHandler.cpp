// Fill out your copyright notice in the Description page of Project Settings.



#include "Mass/Replication/BubbleHandler/TransformSmoothMassClientBubbleHandler.h"
#include "Mass/Fragments/TransformOffsetFragment.h"
#include "Mass/Fragments/TransformOffsetParams.h"

#if UE_REPLICATION_COMPILE_CLIENT_CODE  
void UTransformSmoothMassClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView,
                                                                    const FTransformReplicatedAgent& Item)
{
	FTransformFragment& TransformFragment = EntityView.GetFragmentData<FTransformFragment>();
  	
	const FVector PreviousLocation = TransformFragment.GetTransform().GetLocation();
	const float PreviousYaw = FRotator::NormalizeAxis(TransformFragment.GetTransform().Rotator().Yaw);
	FTransformMassClientBubbleHandler::PostReplicatedChangeEntity(EntityView, Item);
	const FVector NewLocation = TransformFragment.GetTransform().GetLocation();
	const float NewYaw = FRotator::NormalizeAxis(TransformFragment.GetTransform().Rotator().Yaw);
  
 
	// Initializes mesh offset 
	FTransformOffsetFragment& TranslationOffset = EntityView.GetFragmentData<FTransformOffsetFragment>();
	const FTransformOffsetParams& OffsetParams = EntityView.GetConstSharedFragmentData<FTransformOffsetParams>();
  	
	if (OffsetParams.MaxSmoothNetUpdateDistanceSqr > FVector::DistSquared(PreviousLocation, NewLocation))
	{
		// Offsetting the mesh to sync with the sever locations smoothly
		TranslationOffset.TransformOffset.SetPosition( TranslationOffset.TransformOffset.GetPosition()+ PreviousLocation - NewLocation);
	}
	
	const float DeltaYaw = FMath::FindDeltaAngleDegrees(NewYaw, PreviousYaw);
	if (OffsetParams.MaxSmoothNetUpdateYaw > FMath::Abs(DeltaYaw))
	{
		// 기존 오프셋에 최단 거리 각도 차이를 더해줌
		float CurrentYawOffset = TranslationOffset.TransformOffset.GetYaw();
		TranslationOffset.TransformOffset.SetYaw(FRotator::NormalizeAxis(CurrentYawOffset + DeltaYaw));
	}
	
}
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE  
