#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "TWBuildingTypes.h"
#include "TWUnitStatus.h"
#include "TWUnitTableRowBase.generated.h"


USTRUCT(BlueprintType)
struct  CH4_PROJECT_API FTWUnitTableRowBase : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit")
	FName UnitID;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit")
	TObjectPtr<UMassEntityConfigAsset> MassEntityConfigAsset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit")
	FTWUnitStatus BastStatus;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit")
	int32 Population = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit")
	TMap<EResourceType, int32> Cost;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit")
	float SpawnDuration = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Unit")
	TObjectPtr<UTexture2D> Icon;	

};
