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
};