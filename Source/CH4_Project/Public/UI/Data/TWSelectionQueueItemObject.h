#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWSelectionQueueItemObject.generated.h"

UCLASS()
class CH4_PROJECT_API UTWSelectionQueueItemObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FProductionQueueItemViewModel QueueData;

	UPROPERTY(BlueprintReadOnly)
	int32 QueueIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	int32 TotalCount = 0;
};