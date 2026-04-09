#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWSelectionSummaryItemObject.generated.h"

UCLASS(BlueprintType)
class CH4_PROJECT_API UTWSelectionSummaryItemObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Selection")
	FSelectionSummaryItemViewModel SummaryData;
};