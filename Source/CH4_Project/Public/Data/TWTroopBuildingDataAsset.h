#pragma once

#include "CoreMinimal.h"
#include "TWBuildingDataAsset.h"
#include "TWTroopBuildingDataAsset.generated.h"


UCLASS()
class CH4_PROJECT_API UTWTroopBuildingDataAsset : public UTWBuildingDataAsset
{
	GENERATED_BODY()
	
public:
	// 스폰할 병력 블루프린트 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Troop")
	TSubclassOf<AActor> SpawnActorClass;
	
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
