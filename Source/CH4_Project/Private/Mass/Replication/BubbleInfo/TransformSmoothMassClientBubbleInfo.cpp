// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Replication/BubbleInfo/TransformSmoothMassClientBubbleInfo.h"

ATransformSmoothMassClientBubbleInfo::ATransformSmoothMassClientBubbleInfo(
	const FObjectInitializer& ObjectInitializer): AMassClientBubbleInfoBase(ObjectInitializer)
{
	Serializers.Add(&BubbleSerializer);
	SetNetUpdateFrequency(300);
}

void ATransformSmoothMassClientBubbleInfo::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
  
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
  
	// Technically, this doesn't need to be PushModel based because it's a FastArray and they ignore it.
	DOREPLIFETIME_WITH_PARAMS_FAST(ATransformSmoothMassClientBubbleInfo, BubbleSerializer, SharedParams);
}
