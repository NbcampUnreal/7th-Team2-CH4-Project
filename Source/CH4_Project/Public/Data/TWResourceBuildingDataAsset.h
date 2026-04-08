#pragma once

#include "CoreMinimal.h"
#include "TWBuildingDataAsset.h"
#include "TWResourceBuildingDataAsset.generated.h"


UCLASS()
class CH4_PROJECT_API UTWResourceBuildingDataAsset : public UTWBuildingDataAsset
{
	GENERATED_BODY()
	
public:
	// 자원 종류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Resource")
	EResourceType ProducedResourceType = EResourceType::None;
	// 생산 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Resource", meta=(ClampMin="0.1"))
	float ProductionInterval = 5.0f;
	// 생산량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Resource", meta=(ClampMin="1"))
	int32 ProductionAmount = 10;
};
