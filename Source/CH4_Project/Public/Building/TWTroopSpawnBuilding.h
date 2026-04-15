#pragma once

#include "CoreMinimal.h"
#include "TWBaseBuilding.h"
#include "TWTroopSpawnBuilding.generated.h"

class USceneComponent;
class ATWPlayerController;
struct FTWUnitTableRowBase;

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

	// 원본 void 오버로드 제거
	UFUNCTION(BlueprintCallable, Category="Production")
	bool RequestEnqueueTroop();

	UFUNCTION(BlueprintCallable, Category="Production")
	bool RequestEnqueueTroopById(FName UnitId);
	
	void SetQueuePausedByUpkeep(bool bInPaused);

protected:	
	void CancelQueuedProductionAndRestorePendingPopulation();
	virtual void ClearAllBuildingTimers() override;
	virtual void OnOwnerPlayerStateAssigned() override;

public:
	UFUNCTION(BlueprintCallable, Category="Production")
	int32 GetCurrentQueueCount() const { return ProductionQueue.Num(); }

	UFUNCTION(BlueprintCallable, Category="Production")
	const TArray<FName>& GetQueuedUnitIds() const { return ProductionQueue; }

	UFUNCTION(BlueprintCallable, Category="Production")
	bool IsProducing() const { return bIsProducing; }

	UFUNCTION(BlueprintCallable, Category="Production")
	bool IsQueuePausedByUpkeep() const { return bQueuePausedByUpkeep; }

	UFUNCTION(BlueprintCallable, Category="Production")
	FName GetCurrentProducingUnitId() const { return CurrentProducingUnitId; }

	UFUNCTION(BlueprintCallable, Category="Production")
	float GetCurrentProducingDuration() const { return CurrentProducingDuration; }

	UFUNCTION(BlueprintCallable, Category="Production")
	float GetCurrentProductionProgressRatio() const;

	UFUNCTION(BlueprintCallable, Category="Production")
	FString GetCurrentProductionProgressText() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Production")
	TArray<FName> ProductionQueue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Production")
	bool bIsProducing = false;

	// uint8 원본 흐름과 별개로, 실제 생산 로직용 bool 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Production")
	bool bQueuePausedByUpkeep = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Production")
	FName CurrentProducingUnitId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Production")
	float CurrentProducingDuration = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category="Production")
	float CurrentProductionStartTime = 0.f;

	FTimerHandle ProductionTimerHandle;

	const UObject* GetTroopBuildingDataObject() const;
	bool ResolveTrainableUnitIds(TArray<FName>& OutTrainableUnitIds) const;
	bool ResolveDefaultUnitId(FName& OutUnitId) const;
	int32 ResolveMaxQueueCount() const;
	bool CanEnqueueUnitId(FName UnitId, FString* OutFailReason = nullptr) const;
	const FTWUnitTableRowBase* FindUnitRow(FName UnitId) const;
	void TryStartNextProduction();
	void StartProductionForUnit(FName UnitId, const FTWUnitTableRowBase& UnitRow);
	void HandleProductionFinished();
	FVector ResolveSpawnLocation() const;
};