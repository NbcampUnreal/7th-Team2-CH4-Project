#pragma once

#include "CoreMinimal.h"
#include "TWBaseBuilding.h"
#include "TWTroopSpawnBuilding.generated.h"

class USceneComponent;
class UTWTroopBuildingDataAsset;

UCLASS()
class CH4_PROJECT_API ATWTroopSpawnBuilding : public ATWBaseBuilding
{
	GENERATED_BODY()

public:
	ATWTroopSpawnBuilding();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawn")
	TObjectPtr<USceneComponent> SpawnPoint = nullptr;
	
	void RequestEnqueueTroop();
	int8 SpawnUnitNow();
	
protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Spawn|Queue")
	int32 CurrentQueueCount = 0;

	FTimerHandle SpawnQueueTimerHandle;

protected:
	const UTWTroopBuildingDataAsset* GetTroopBuildingData() const;

	void StartSpawnQueueTimer();
	void HandleSpawnQueue();
	
	virtual void ClearAllBuildingTimers() override;
};