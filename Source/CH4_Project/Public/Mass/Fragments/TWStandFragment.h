#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "TWStandFragment.generated.h"

USTRUCT()
struct FTWStandFragment : public FMassFragment
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool bIsStopping = false;
};
