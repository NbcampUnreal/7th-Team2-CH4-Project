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
	UFUNCTION(BlueprintCallable, Category="Population|Queue")
	int32 RequestEnqueuePopulation();
	
	UFUNCTION(BlueprintCallable, Category="Population|Queue")
	int32 IncreasePopulationNow();
	
	UFUNCTION(BlueprintCallable, Category="Population|Queue")
	int32 GetCurrentQueueCount() const { return CurrentQueueCount; }

	UFUNCTION(BlueprintCallable, Category="Population|Queue")
	bool IsProducing() const { return bIsProducing; }

	UFUNCTION(BlueprintCallable, Category="Population|Queue")
	float GetCurrentProducingDuration() const { return CurrentProducingDuration; }

	UFUNCTION(BlueprintCallable, Category="Population|Queue")
	float GetCurrentProductionProgressRatio() const;

	UFUNCTION(BlueprintCallable, Category="Population|Queue")
	FString GetCurrentProductionProgressText() const;

protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Population|Queue")
	int32 CurrentQueueCount = 0;
	
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Population|Queue")
	bool bIsProducing = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Population|Queue")
	float CurrentProducingDuration = 0.f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Population|Queue")
	float CurrentProductionStartTime = 0.f;

	FTimerHandle PopulationQueueTimerHandle;

protected:
	const UTWPopulationBuildingDataAsset* GetPopulationBuildingData() const;

	void TryStartNextProduction();
	void HandlePopulationQueue();
	
	void CancelQueuedPopulation();
	virtual void ClearAllBuildingTimers() override;
};
