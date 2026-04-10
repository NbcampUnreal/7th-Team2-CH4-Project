#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "TWBuildingTypes.h"
#include "TWUnitStatus.generated.h"

UENUM()
enum class ETWStatusType : uint8
{
	Health,
	Damage,
	Armor,
	AttackSpeed,
	MoveSpeed,
	Range,
	Count
};
USTRUCT(BlueprintType)
struct  CH4_PROJECT_API FTWUnitStatus
{
	GENERATED_BODY()
	FTWUnitStatus()
	{
		for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); i++)
		{
			Status[i] = 0.0f;
		}
	}
	
	float GetStatus(ETWStatusType TargetStatus)const {return Status[static_cast<int32>(TargetStatus)];}
	void SetStatus(ETWStatusType TargetStatus, float Value){Status[static_cast<int32> (TargetStatus)] = Value;}
	float Status[static_cast<int32>(ETWStatusType::Count)] = {0.0f,};

};
