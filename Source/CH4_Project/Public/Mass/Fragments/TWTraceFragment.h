#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "MassEntityElementTypes.h"
#include "TWTraceFragment.generated.h"

USTRUCT()
struct FTWTraceFragment : public FMassFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Trace")
	float DetectionRadius = 0.0f;
	
	FMassEntityHandle TargetEntity;
};
