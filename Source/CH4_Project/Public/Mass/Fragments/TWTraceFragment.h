#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "MassEntityElementTypes.h"
#include "TWTraceFragment.generated.h"

USTRUCT()
struct FTWTraceFragment : public FMassFragment
{
	GENERATED_BODY()
	
	// 공격 사거리x / 타겟 인지 사거리o
	UPROPERTY(EditAnywhere, Category = "Trace")
	float DetectionRadius = 0.0f;
	
	FMassEntityHandle TargetEntity;
};