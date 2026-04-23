#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "Building/TWBaseBuilding.h"
#include "TWAttackFragment.generated.h"

USTRUCT()
struct FTWAttackFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FTWAttackFragment() = default;
	float LastSearchTime = 0.0f;
	float LastAttackTime = 0.0f;
	float LastChasingTime = 0.0f;
	float LastAttackAnimationTime = 0.0f;
public:
	void SetTargetEntity(const FMassEntityHandle& InTargetEntity){ClearTarget(); TargetEntity = InTargetEntity; bIsTargetSet = true;}
	void SetTargetBuilding(ATWBaseBuilding* InTargetBuilding){ClearTarget(); TargetBuilding = InTargetBuilding; bIsTargetSet = true;}
	FMassEntityHandle GetTargetEntity() const {return TargetEntity;}
	ATWBaseBuilding* GetTargetBuilding() const {return TargetBuilding.Get();}
	void ClearTarget(){bIsTargetSet = false; TargetEntity = FMassEntityHandle(); TargetBuilding.Reset();}
	
	uint8 GetIsTargetSet() const {return bIsTargetSet;}
private:
	FMassEntityHandle TargetEntity;
	TWeakObjectPtr<ATWBaseBuilding> TargetBuilding;
	
	uint8 bIsTargetSet = false;
	
};
