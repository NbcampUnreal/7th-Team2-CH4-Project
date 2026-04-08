#pragma once

#include "CoreMinimal.h"
#include "TWBuildingDataAsset.h"
#include "TWBlockingBuildingDataAsset.generated.h"

UCLASS()
class CH4_PROJECT_API UTWBlockingBuildingDataAsset : public UTWBuildingDataAsset
{
	GENERATED_BODY()

public:
	// 최대 체력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Blocking", meta=(ClampMin="1"))
	int32 MaxHP = 100;
};