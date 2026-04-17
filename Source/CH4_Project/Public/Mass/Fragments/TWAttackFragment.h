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
	//Last Searching Time
	float LastAttackTime = 0.0f;
	uint8 bIsTargetSet = false;
	
};
