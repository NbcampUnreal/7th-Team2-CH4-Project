// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_6
#include "MassObserverProcessor.h"
#endif // UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_6
#include "TWSteeringProcessors.generated.h"

/** 
* Processor for updating steering towards MoveTarget.
*/
UCLASS()
class UTWSteerToMoveTargetProcessor : public UMassProcessor
{
	GENERATED_BODY()

protected:
	UTWSteerToMoveTargetProcessor();
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
