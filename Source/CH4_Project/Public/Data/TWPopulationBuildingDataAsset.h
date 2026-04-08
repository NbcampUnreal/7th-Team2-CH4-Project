#pragma once

#include "CoreMinimal.h"
#include "TWBuildingDataAsset.h"
#include "TWPopulationBuildingDataAsset.generated.h"

UCLASS()
class CH4_PROJECT_API UTWPopulationBuildingDataAsset : public UTWBuildingDataAsset
{
	GENERATED_BODY()

public:
	// 생산 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Population", meta=(ClampMin="0.1"))
	float ProductionInterval = 3.0f;

	// 최대 대기열 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Population", meta=(ClampMin="1"))
	int32 MaxQueueCount = 5;
	
	// 생산 비용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Population")
	FBuildingResourceCost ProductionCost;
};