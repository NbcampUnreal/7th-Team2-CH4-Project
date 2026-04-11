#pragma once

#include "CoreMinimal.h"
#include "TWBuildingDataAsset.h"
#include "TWTroopBuildingDataAsset.generated.h"

class UMassEntityConfigAsset;

UCLASS()
class CH4_PROJECT_API UTWTroopBuildingDataAsset : public UTWBuildingDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop")
	TArray<FName> TrainableUnitIDs;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop")
	FName DefaultUnitID = NAME_None;
	
	// 최대 대기열 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop", meta=(ClampMin="1"))
	int32 MaxQueueCount = 5;
};