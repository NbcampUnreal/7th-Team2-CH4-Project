#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "MassEntityQuery.h"
#include "Data/TWBuildingTypes.h"
#include "Data/TWUnitStatus.h"
#include "Subsystems/WorldSubsystem.h"
#include "TWUnitSubsystem.generated.h"

class UTWPlayerUnitContainer;
struct FTWUnitTableRowBase;
class ATWPlayerController;
struct FMassEntityHandle;

UCLASS()
class CH4_PROJECT_API UTWUnitSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UTWUnitSubsystem();
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;

#ifdef WITH_SERVER_CODE
	void AddPlayer(int32 PlayerSlot);

	bool FindNearestEntity(
		const FVector& Location,
		FMassEntityHandle& OutEntityHandle,
		int32 OwnerPlayerSlot,
		float MaxDistance = 50.0f
	);

	bool GetEntitiesInRectangle(
		const FVector& StartLocation,
		const FVector& EndLocation,
		TArray<FMassEntityHandle>& OutEntityHandles,
		int32 OwnerPlayerSlot
	);

	void SpawnUnit(
		const FVector& Location,
		const FTWUnitTableRowBase& UnitTableRowBase,
		ATWPlayerController* PlayerController
	);

	TMap<EResourceType, int32> GetUpkeep(int32 PlayerSlot);
	int32 GetCurrentPopulation(int32 PlayerSlot) const;

private:
	void AddUnit(int32 PlayerSlot, FMassEntityHandle& Unit);
	void RemoveUnit(int32 PlayerSlot, int32 Idx);
#endif

public:
	//업그레이드까지 적용된 풀컨디션 스탯임 BaseStat아님
	FTWUnitStatus GetUnitDefaultStatus(FName UnitID, int32 PlayerSlot);
	FTWUnitStatus GetUnitCurrentStatus(const FMassEntityHandle& Unit, const int32 PlayerSlot);
#ifdef WITH_CLIENT_CODE
	FTWUnitStatus GetUnitCurrentStatus(const FMassNetworkID& UnitNetID, const int32 PlayerSlot);
#endif
	
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