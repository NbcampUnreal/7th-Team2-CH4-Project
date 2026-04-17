#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "TWClientVelocityFragment.generated.h"

USTRUCT()
struct FTWClientVelocityFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FTWClientVelocityFragment() = default;
	FVector Velocity;
	
};
