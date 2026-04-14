#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "TWAttackFragment.generated.h"

USTRUCT()
struct FTWAttackFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FTWAttackFragment() = default;
	
	UPROPERTY(EditAnywhere, Category="Attack")
	float AttackInterval = 0.0f;
	
	float LastAttackTime = 0.0f;
};
