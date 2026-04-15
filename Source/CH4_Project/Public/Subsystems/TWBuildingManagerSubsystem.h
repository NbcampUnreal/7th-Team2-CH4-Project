#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TWBuildingManagerSubsystem.generated.h"


class ATWBaseBuilding;

USTRUCT(BlueprintType)
struct FTWPlayerBuildingData
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<ATWBaseBuilding*> Buildings;
};

UCLASS()
class CH4_PROJECT_API UTWBuildingManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable, Category="Building|Management")
	void RegisterBuilding(int32 PlayerSlot, ATWBaseBuilding* Building);
	
	UFUNCTION(BlueprintCallable, Category="Building|Management")
	void UnregisterBuilding(int32 PlayerSlot, ATWBaseBuilding* Building);
	
	UFUNCTION(BlueprintCallable, Category="Building|Management")
	TArray<ATWBaseBuilding*> GetWorldBuildingsByPlayer(int32 PlayerSlot) const;
	
	const TMap<int32, FTWPlayerBuildingData>& GetAllPlayerBuildingsMap() const 
	{ 
		return WorldBuildingMap; 
	}
	
	UFUNCTION(BlueprintCallable, Category = "Building|Management")
	TArray<ATWBaseBuilding*> GetAllBuildings() const;	// ID 상관없이 건물 전체 반환
private:
	UPROPERTY()
	TMap<int32, FTWPlayerBuildingData> WorldBuildingMap;
};
