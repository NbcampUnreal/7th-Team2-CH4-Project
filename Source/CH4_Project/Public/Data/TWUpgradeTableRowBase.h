#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "TWBuildingTypes.h"
#include "TWUnitStatus.h"
#include "TWUpgradeTableRowBase.generated.h"


USTRUCT(BlueprintType)
struct  CH4_PROJECT_API FTWUpgradeTableRowBase : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	FName UpgradeID;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	ETWStatusType TargetStatus = ETWStatusType::Damage;
	//적용대상 유닛 ID, 적용수치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	TMap<FName, int32> TargetUnits;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	TMap<EResourceType, int32> Cost;
	//업그레이드 할 떄 마다 비싸지는 비용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	TMap<EResourceType, int32> CostIncreaseAmount;
	//업그레이드 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	float Duration = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<UTexture2D> Icon;	

};
