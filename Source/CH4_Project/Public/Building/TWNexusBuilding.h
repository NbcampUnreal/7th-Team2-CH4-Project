#pragma once

#include "CoreMinimal.h"
#include "Building/TWBlockingBuilding.h"
#include "TWNexusBuilding.generated.h"

class UTWNexusBuildingDataAsset;

UCLASS()
class CH4_PROJECT_API ATWNexusBuilding : public ATWBlockingBuilding
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	virtual void ApplyDamageToBuilding(const int32 InDamageAmount) override;

protected:
	FTimerHandle RegenDelayTimerHandle;
	FTimerHandle RegenTickTimerHandle;

protected:
	const UTWNexusBuildingDataAsset* GetNexusBuildingData() const;

	void StartHPRegen();
	void HandleHPRegen();

	virtual void HandleDestroyedByDamage() override;
	virtual void ClearAllBuildingTimers() override;
};