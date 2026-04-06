// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TransformMassClientBubbleHandler.h"
#include "UObject/Object.h"

/**
 * 
 */
class CH4_PROJECT_API UTransformSmoothMassClientBubbleHandler : public FTransformMassClientBubbleHandler
{
protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE 
	virtual void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FTransformReplicatedAgent& Item) override;
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
	
};
