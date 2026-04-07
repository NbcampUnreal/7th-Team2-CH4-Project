// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TWTransformMassClientBubbleHandler.h"
#include "UObject/Object.h"

/**
 * 
 */
class CH4_PROJECT_API UTWTransformSmoothMassClientBubbleHandler : public FTWTransformMassClientBubbleHandler
{
protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE 
	virtual void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FTWTransformReplicatedAgent& Item) override;
#endif //UE_REPLICATION_COMPILE_CLIENT_CODE
	
};
