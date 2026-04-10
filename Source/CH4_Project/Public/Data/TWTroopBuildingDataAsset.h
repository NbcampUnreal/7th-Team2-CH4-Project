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
	// 스폰할 병력 Mass Entity Config
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop")
	TObjectPtr<UMassEntityConfigAsset> UnitEntityConfig = nullptr;
	
	// 생산 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop", meta=(ClampMin="0.1"))
	float SpawnInterval = 3.0f;

	// 최대 대기열 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop", meta=(ClampMin="1"))
	int32 MaxQueueCount = 5;
	
	// 생산 비용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop")
	FBuildingResourceCost SpawnCost;
};
