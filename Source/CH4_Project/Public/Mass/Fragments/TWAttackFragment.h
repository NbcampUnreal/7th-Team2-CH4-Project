#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "TWAttackFragment.generated.h"

USTRUCT()
struct FTWAttackFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FTWAttackFragment() = default;
	
	float LastAttackTime = 0.0f;
};
