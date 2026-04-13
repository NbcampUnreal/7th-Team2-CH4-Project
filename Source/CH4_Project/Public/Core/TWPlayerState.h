#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "Core/TWPlayerUnitContainer.h"
#include "MassEntityHandle.h"
#include "GameFramework/PlayerState.h"
#include "Data/TWBuildingTypes.h"
#include "Data/TWUnitStatus.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "TWPlayerState.generated.h"

UCLASS()
class CH4_PROJECT_API ATWPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATWPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Player")
	int32 PlayerSlot = -1;

	void SetPlayerSlot(const int32 InPlayerSlot);

#pragma region 자원
protected:
	UPROPERTY(ReplicatedUsing=OnRep_Resources, BlueprintReadOnly, Category="Resource")
	TArray<int32> Resources;

	UFUNCTION()
	void OnRep_Resources();

public:
	void AddResource(const EResourceType ResourceType, const int32 Amount);

	FORCEINLINE int32 GetResourceAmount(const EResourceType ResourceType) const
	{
		const int32 Index = static_cast<int32>(ResourceType);
		return Resources.IsValidIndex(Index) ? Resources[Index] : 0;
	}
#pragma endregion

#pragma region 인구 / 병력
protected:
	UPROPERTY(ReplicatedUsing=OnRep_PendingPopulation, VisibleAnywhere, BlueprintReadOnly, Category="Population")
	int32 PendingPopulation = 0;

	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category="Population")
	int32 MaxPopulation = 200;

	UPROPERTY(ReplicatedUsing=OnRep_PopulationLimit, VisibleAnywhere, BlueprintReadOnly, Category="Population")
	int32 PopulationLimit = 1;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentPopulation, VisibleAnywhere, BlueprintReadOnly, Category="Population")
	int32 CurrentPopulation = 0;

	UFUNCTION()
	void OnRep_PendingPopulation();

	UFUNCTION()
	void OnRep_PopulationLimit();

	UFUNCTION()
	void OnRep_CurrentPopulation();

public:
	int8 CanAffordCost(const TMap<EResourceType, int32>& Cost) const;
	void SpendCost(const TMap<EResourceType, int32>& Cost);

	int8 CanQueueTroop(const int32 RequiredPopulation) const;

	void SetCurrentPopulationFromContainer(const int32 InAmount);

	void AddPendingPopulation(const int32 InAmount);
	void RemovePendingPopulation(const int32 InAmount);

	void AddPopulationLimit(const int32 InAmount);

	FORCEINLINE int32 GetPendingPopulation() const { return PendingPopulation; }
	FORCEINLINE int32 GetMaxPopulation() const { return MaxPopulation; }
	FORCEINLINE int32 GetPopulationLimit() const { return PopulationLimit; }
	FORCEINLINE int32 GetCurrentPopulation() const { return CurrentPopulation; }

#pragma endregion

#pragma region 병력 유지비
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Troop|Upkeep")
	TMap<EResourceType, int32> UpkeepCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Troop|Upkeep", meta=(ClampMin="0.1"))
	float UpkeepInterval = 60.0f;

	FTimerHandle TroopUpkeepTimerHandle;

public:
	FORCEINLINE const TMap<EResourceType, int32>& GetTotalTroopUpkeepCost() const { return UpkeepCost; }
	void SetTotalTroopUpkeepCost(const TMap<EResourceType, int32>& Upkeep) { UpkeepCost = Upkeep; }
	void RefreshTroopUpkeepTimer();
	void HandleTroopUpkeep();
	int8 TrySpendTroopUpkeep();
#pragma endregion
	
#pragma region 업그레이드
protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	TArray<int32> StatusUpgradeLevels;

public:
	int32 GetStatusUpgradeLevel(const ETWStatusType StatusType) const;
	void AddStatusUpgradeLevel(const ETWStatusType StatusType, const int32 InAmount = 1);
#pragma endregion
	

private:
	void NotifyUIResourceStateChanged();

private:
	FORCEINLINE UTWUnitSubsystem* GetUnitSubsystem() const
	{
		return GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	}
};