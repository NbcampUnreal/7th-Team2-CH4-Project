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

struct FTWUpgradeTableRowBase;
class UTWTeamComponent;

USTRUCT(BlueprintType)
struct FTWUpgradeBonusDelta
{
	GENERATED_BODY()

	UPROPERTY()
	FName UnitID = NAME_None;

	UPROPERTY()
	uint8 StatusType = 0;

	UPROPERTY()
	float NewValue = 0.f;
};

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
	int32 PopulationLimit = 10;

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

	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedWoodUpkeep, VisibleAnywhere, BlueprintReadOnly, Category="Troop|Upkeep")
	int32 ReplicatedWoodUpkeep = 0;

	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedOreUpkeep, VisibleAnywhere, BlueprintReadOnly, Category="Troop|Upkeep")
	int32 ReplicatedOreUpkeep = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Troop|Upkeep", meta=(ClampMin="0.1"))
	float UpkeepInterval = 60.0f;

	FTimerHandle TroopUpkeepTimerHandle;

	UFUNCTION()
	void OnRep_ReplicatedWoodUpkeep();

	UFUNCTION()
	void OnRep_ReplicatedOreUpkeep();

public:
	FORCEINLINE const TMap<EResourceType, int32>& GetTotalTroopUpkeepCost() const { return UpkeepCost; }

	void SetTotalTroopUpkeepCost(const TMap<EResourceType, int32>& Upkeep);

	FORCEINLINE int32 GetReplicatedWoodUpkeep() const { return ReplicatedWoodUpkeep; }
	FORCEINLINE int32 GetReplicatedOreUpkeep() const { return ReplicatedOreUpkeep; }

	void RefreshTroopUpkeepTimer();
	void HandleTroopUpkeep();
	int8 TrySpendTroopUpkeep();
#pragma endregion
	
#pragma region 업그레이드
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	TMap<FName, int32> UpgradeLevelsByID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Upgrade")
	TMap<FName, FTWUnitStatus> UnitUpgradeBonusMap;
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastApplyUpgradeBonusDeltas(const TArray<FTWUpgradeBonusDelta>& InDeltas);

public:
	int32 GetUpgradeLevelByID(const FName UpgradeID) const;
	void ApplyUpgradeRow(const FTWUpgradeTableRowBase& UpgradeRow);
	float GetUnitUpgradeBonus(const FName UnitID, const ETWStatusType StatusType) const;
	FTWUnitStatus GetUnitUpgradeBonus(const FName UnitID) const;
#pragma endregion
	
#pragma region 팀
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Team")
	TObjectPtr<UTWTeamComponent> TeamComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category="Team")
	int32 GetTeamID() const;

	UFUNCTION(BlueprintCallable, Category="Team")
	void SetTeamID(int32 InTeamID);
#pragma endregion
	
#pragma region 승/패 판정
protected:
	UPROPERTY(Replicated)
	int32 GameResult;
	
public:
	void SetGameResult(int32 InResult) {GameResult = InResult;}
	FORCEINLINE int32 GetGameResult() const {return GameResult;}
#pragma endregion
	
private:
	void NotifyUIResourceStateChanged();

private:
	FORCEINLINE UTWUnitSubsystem* GetUnitSubsystem() const
	{
		return GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	}
};	