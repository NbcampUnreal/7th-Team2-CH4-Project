#pragma once

#include "CoreMinimal.h"
#include "Data/TWBlockingBuildingDataAsset.h"
#include "TWNexusBuildingDataAsset.generated.h"

UCLASS()
class CH4_PROJECT_API UTWNexusBuildingDataAsset : public UTWBlockingBuildingDataAsset
{
	GENERATED_BODY()

public:
	// 마지막 피격 후 일정 시간 후 회복
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Nexus", meta=(ClampMin="0.1"))
	float RegenDelay = 1.0f;
	
	// 자연 회복 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Nexus", meta=(ClampMin="0.1"))
	float RegenInterval = 1.0f;

	// 재생량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Nexus", meta=(ClampMin="1"))
	int32 RegenAmount = 50;
};