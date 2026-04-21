#pragma once

#include "CoreMinimal.h"
#include "TWBaseBuilding.h"
#include "Data/TWUnitStatus.h"
#include "TWUpgradeBuilding.generated.h"

class UDataTable;
struct FTWUpgradeTableRowBase;

UCLASS()
class CH4_PROJECT_API ATWUpgradeBuilding : public ATWBaseBuilding
{
	GENERATED_BODY()
	
public:
	ATWUpgradeBuilding();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	int8 RequestStartUpgrade(const FName InUpgradeID = NAME_None);

	UFUNCTION(BlueprintCallable)
	const TArray<FName>& GetQueuedUpgradeIds() const
	{
		return UpgradeQueue;
	}

	UFUNCTION(BlueprintCallable)
	bool IsProducing() const
	{
		return bIsUpgradeInProgress != 0;
	}

	UFUNCTION(BlueprintCallable)
	int32 GetCurrentQueueCount() const
	{
		return UpgradeQueue.Num();
	}

	UFUNCTION(BlueprintCallable)
	float GetCurrentProductionProgressRatio() const;

	UFUNCTION(BlueprintCallable)
	FString GetCurrentProductionProgressText() const;

	UFUNCTION(BlueprintCallable)
	bool ResolveAvailableUpgradeIds(TArray<FName>& OutUpgradeIds) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<UDataTable> UpgradeTable = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	int32 MaxUpgradeQueueCount = 1;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	uint8 bIsUpgradeInProgress = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	FName CurrentUpgradeID = NAME_None;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	TArray<FName> UpgradeQueue;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	float CurrentUpgradeDuration = 0.f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	float CurrentUpgradeStartTime = 0.f;

	FTimerHandle UpgradeTimerHandle;

protected:
	FTWUpgradeTableRowBase* GetUpgradeRow(const FName InUpgradeID) const;
	TMap<EResourceType, int32> BuildCurrentUpgradeCost(const FTWUpgradeTableRowBase& UpgradeRow) const;

	int32 CountQueuedSameUpgrade(const FName InUpgradeID) const;
	void TryStartNextUpgrade();
	void StartUpgradeInternal(const FName InUpgradeID, const FTWUpgradeTableRowBase& UpgradeRow);
	void FinishUpgrade();

	virtual void ClearAllBuildingTimers() override;
};