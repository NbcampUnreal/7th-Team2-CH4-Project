#pragma once

#include "CoreMinimal.h"
#include "Building/TWBaseBuilding.h"
#include "Data/TWBuildingTypes.h"
#include "TWResourceBuilding.generated.h"

class UTWResourceBuildingDataAsset;
class ATWFloatingTextActor;

UCLASS()
class CH4_PROJECT_API ATWResourceBuilding : public ATWBaseBuilding
{
	GENERATED_BODY()

public:
	const UTWResourceBuildingDataAsset* GetResourceBuildingData() const;

protected:
	virtual void OnOwnerPlayerStateAssigned() override;
	virtual void ClearAllBuildingTimers() override;

	void StartResourceProduction();
	void HandleProduceResource();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_ShowResourceText(EResourceType InResourceType, int32 InAmount);

	void SpawnResourceText(EResourceType InResourceType, int32 InAmount);

	bool ShouldShowResourceTextOnThisClient() const;
	FString MakeResourceText(EResourceType InResourceType, int32 InAmount) const;
	FColor GetResourceTextColor(EResourceType InResourceType) const;

protected:
	UPROPERTY()
	FTimerHandle ResourceProductionTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<ATWFloatingTextActor> FloatingTextActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float FloatingTextZOffset = 40.0f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float FloatingTextRandomXYOffset = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float FloatingTextLifetime = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float FloatingTextRiseSpeed = 35.0f;
};