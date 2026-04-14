#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "TWAnimPlayFragment.generated.h"

USTRUCT()
struct FTWAnimPlayFragment : public FMassFragment
{
	GENERATED_BODY()
	
	UPROPERTY()
	bool bIsAttackAnimPlay = false;
	
	UPROPERTY()
	bool bIsMovingAnimPlay = false;
	
	UPROPERTY()
	bool bIsIdleAnimPlay = false;
	
	UPROPERTY()
	bool bIsDeadAnimPlay = false;
};
