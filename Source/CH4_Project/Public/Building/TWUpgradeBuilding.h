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

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Upgrade")
	TObjectPtr<UDataTable> UpgradeTable = nullptr;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	uint8 bIsUpgradeInProgress = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	FName CurrentUpgradeID = NAME_None;

	FTimerHandle UpgradeTimerHandle;

protected:
	FTWUpgradeTableRowBase* GetUpgradeRow(const FName InUpgradeID) const;
	TMap<EResourceType, int32> BuildCurrentUpgradeCost(const FTWUpgradeTableRowBase& UpgradeRow) const;

	void FinishUpgrade();
	virtual void ClearAllBuildingTimers() override;
};