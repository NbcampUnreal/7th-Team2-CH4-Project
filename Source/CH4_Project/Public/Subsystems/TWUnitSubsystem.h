// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityQuery.h"
#include "Data/TWBuildingTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "TWUnitSubsystem.generated.h"

/**
 * 
 */
class UTWPlayerUnitContainer;
struct FTWUnitTableRowBase;
class ATWPlayerController;
class UMassEntityConfigAsset;
struct FMassEntityHandle;

UCLASS()
class CH4_PROJECT_API UTWUnitSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	// UTWUnitSubsystem();
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;
	
#ifdef WITH_SERVER_CODE
	void AddPlayer(int32 PlayerSlot);
	bool FindNearestEntity(const FVector& Location, FMassEntityHandle& OutEntityHandle, float MaxDistance = 50.0f);
	bool GetEntitiesInRectangle(const FVector& StartLocation, const FVector& EndLocation, TArray<FMassEntityHandle>& OutEntityHandles);
	void SpawnUnit(const FVector& Location, const FTWUnitTableRowBase& UnitTableRowBase, ATWPlayerController* PlayerController);
	TMap<EResourceType, int32> GetUpkeep(int32 PlayerSlot);
	int32 GetCurrentPopulation(int32 PlayerSlot) const;
	
	
private:
	void AddUnit(int32 PlayerSlot, FMassEntityHandle& Unit);
	void RemoveUnit(int32 PlayerSlot, int32 Idx);
	
	
#endif
public:
	FTWUnitTableRowBase* GetUnitTableRowBase(FName UnitID) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> UnitTable;
private:
	UPROPERTY()
	TMap<int32, UTWPlayerUnitContainer*> UnitContainers; 
	FMassEntityQuery FindNearestEntityQuery;
	TMap<FName, FTWUnitTableRowBase*> CachedUnitTableRows;
};
