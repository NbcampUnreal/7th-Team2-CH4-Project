#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "TWAttackFragment.generated.h"

USTRUCT()
struct FTWAttackFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FTWAttackFragment() = default;
	FMassEntityHandle TargetEntity;
	float LastSearchTime = 0.0f;
	float LastAttackTime = 0.0f;
	float LastAttackAnimationTime = 0.0f;
	uint8 bIsTargetSet = false;
	
};
