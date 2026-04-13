#pragma once

#include "CoreMinimal.h"
#include "TWBaseBuilding.h"
#include "TWTroopSpawnBuilding.generated.h"

class USceneComponent;
class UTWTroopBuildingDataAsset;

USTRUCT(BlueprintType)
struct FTWQueuedTroopData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawn|Queue")
	FName UnitID = NAME_None;
};

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
	
	void RequestEnqueueTroop(const FName InUnitID = NAME_None);
	int8 SpawnUnitNow();
	
	void SetQueuePausedByUpkeep(const uint8 bInPaused);
	
protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Spawn|Queue")
	int32 CurrentQueueCount = 0;
	
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Spawn|Upkeep")
	uint8 QueuePausedByUpkeep = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawn|Queue")
	TArray<FTWQueuedTroopData> TroopQueue;

	FTimerHandle SpawnQueueTimerHandle;

protected:
	const UTWTroopBuildingDataAsset* GetTroopBuildingData() const;

	void StartSpawnQueueTimer();
	void HandleSpawnQueue();
	
	virtual void ClearAllBuildingTimers() override;
};