#pragma once

#include "CoreMinimal.h"
#include "TWBaseBuilding.h"
#include "TWPopulationBuilding.generated.h"

class UTWPopulationBuildingDataAsset;

UCLASS()
class CH4_PROJECT_API ATWPopulationBuilding : public ATWBaseBuilding
{
	GENERATED_BODY()

public:
	ATWPopulationBuilding();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	int8 RequestEnqueuePopulation();
	int8 IncreasePopulationNow();

protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Population|Queue")
	int32 CurrentQueueCount = 0;

	FTimerHandle PopulationQueueTimerHandle;

protected:
	const UTWPopulationBuildingDataAsset* GetPopulationBuildingData() const;

	void StartPopulationQueueTimer();
	void HandlePopulationQueue();
	
	virtual void ClearAllBuildingTimers() override;
};
