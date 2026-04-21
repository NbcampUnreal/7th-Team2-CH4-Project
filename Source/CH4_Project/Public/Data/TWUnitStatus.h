#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "TWBuildingTypes.h"
#include "TWUnitStatus.generated.h"

UENUM(BlueprintType)
enum class ETWStatusType : uint8
{
	Health UMETA(DisplayName = "Health"),
	Damage UMETA(DisplayName = "Damage"),
	Armor UMETA(DisplayName = "Armor"),
	AttackSpeed	UMETA(DisplayName="AttackSpeed"),
	MoveSpeed UMETA(DisplayName = "MoveSpeed"),
	Range UMETA(DisplayName = "Range"),
	SearchingRange UMETA(DisplayName = "SearchingRange"),
	Count UMETA(Hidden),
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
	
	FTWUnitStatus& operator+=(const FTWUnitStatus& Other)
	{
		for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); i++)
		{
			Status[i] += Other.Status[i];
		}
		return *this;
	}
	
	float GetStatus(ETWStatusType TargetStatus)const {return Status[static_cast<int32>(TargetStatus)];}
	void SetStatus(ETWStatusType TargetStatus, float Value){Status[static_cast<int32> (TargetStatus)] = Value;}
	
	UPROPERTY(EditDefaultsOnly, meta = (EnumIndex = "ETWStatusType"))
	float Status[static_cast<int32>(ETWStatusType::Count)];

};
