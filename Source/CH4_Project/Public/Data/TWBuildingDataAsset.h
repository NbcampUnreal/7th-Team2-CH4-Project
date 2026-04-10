#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TWBuildingTypes.h"
#include "TWBuildingDataAsset.generated.h"

UCLASS(BlueprintType)
class CH4_PROJECT_API UTWBuildingDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	// 건물 아이디
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building")
	FName BuildingID = NAME_None;
	
	//건물 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building")
	FText BuildingName;
	
	// 건물 타입
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building")
	EBuildingCategory BuildingCategory = EBuildingCategory::None;
	
	// 건물 블루프린트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building")
	TSubclassOf<class ATWBaseBuilding> BuildingClass;
	
	// 최대 체력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|HP", meta=(ClampMin="1"))
	int32 MaxHP = 100;
	
	// 건설 시간(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Construction", meta=(ClampMin="0.0"))
	float BuildTime = 0.0f;

	// 건설 비용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Construction")
	FBuildingResourceCost BuildCost;
	
	// 건물 설치 크기(그리드 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Construction")
	FBuildingGridSize GridSize;
};
