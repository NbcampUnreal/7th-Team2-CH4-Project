// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "MassEntityHandle.h"
#include "Data/TWBuildingTypes.h"
#include "UObject/Object.h"
#include "TWPlayerUnitContainer.generated.h"

class ATWPlayerController;
class UMassReplicationSubsystem;
/**
 * 
 */
UCLASS()
class CH4_PROJECT_API UTWPlayerUnitContainer : public UObject
{
	GENERATED_BODY()
	
public:
	void Init(int32 InOwnerSlot);
	void Clear();
	void SetOwnerSlot(int32 InOwnerSlot);
	void AddUnit(FMassEntityHandle& Unit);
	void RemoveUnit(int32 Idx);
	
	
#pragma region Getter
	FMassEntityHandle GetEntityHandle(int32 Idx) const;
	FORCEINLINE int32 GetCurrentUnitCount() const{return Units.Num();}
	FORCEINLINE const TMap<EResourceType, int32>& GetUpkeep() const{return Upkeep;}
#pragma endregion
	
private:
	UPROPERTY()
	int32 OwnerSlot;
	UPROPERTY()
	TArray<FMassNetworkID> Units;
	UPROPERTY()
	TObjectPtr<UMassReplicationSubsystem> MassReplicationSubsystem;
	UPROPERTY()
	TMap<EResourceType, int32> Upkeep;
	
};
