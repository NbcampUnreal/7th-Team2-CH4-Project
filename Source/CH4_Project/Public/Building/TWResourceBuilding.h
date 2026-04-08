#pragma once

#include "CoreMinimal.h"
#include "TWBaseBuilding.h"
#include "TWResourceBuilding.generated.h"

class UTWResourceBuildingDataAsset;

UCLASS()
class CH4_PROJECT_API ATWResourceBuilding : public ATWBaseBuilding
{
	GENERATED_BODY()
	
protected:
	FTimerHandle ResourceProductionTimerHandle;
	
protected:
	const UTWResourceBuildingDataAsset* GetResourceBuildingData() const;

protected:
	virtual void OnOwnerPlayerStateAssigned() override;
	virtual void ClearAllBuildingTimers() override;

	void StartResourceProduction(); // 자원 생산 시작
	void HandleProduceResource(); // 자원 지급
};