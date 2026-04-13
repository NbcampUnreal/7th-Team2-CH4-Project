#pragma once

#include "CoreMinimal.h"
#include "Building/TWBlockingBuilding.h"
#include "TWNexusBuilding.generated.h"

class UTWNexusBuildingDataAsset;

UCLASS()
class CH4_PROJECT_API ATWNexusBuilding : public ATWBaseBuilding
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void ApplyDamageToBuilding(const float InDamageAmount) override;

protected:
	FTimerHandle RegenDelayTimerHandle;
	FTimerHandle RegenTickTimerHandle;
	FTimerHandle WoodProductionTimerHandle;

protected:
	const UTWNexusBuildingDataAsset* GetNexusBuildingData() const;

	void StartHPRegen();
	void HandleHPRegen();
	
	void StartWoodProduction();
	void HandleWoodProduction();

	virtual void OnOwnerPlayerStateAssigned() override;
	virtual void HandleDestroyedByDamage() override;
	virtual void ClearAllBuildingTimers() override;
};