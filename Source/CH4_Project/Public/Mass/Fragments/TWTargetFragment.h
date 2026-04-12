#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "TWTargetFragment.generated.h"

USTRUCT()
struct FTWTargetFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FMassEntityHandle TargetEntity;
	
	bool bIsTargetInRange = false;
};
