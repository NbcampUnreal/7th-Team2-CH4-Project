#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "GameFramework/PlayerState.h"
#include "Data/TWBuildingTypes.h"
#include "TWPlayerState.generated.h"

UCLASS()
class CH4_PROJECT_API ATWPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATWPlayerState();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 0 = Player1, 1 = Player2
	UPROPERTY(ReplicatedUsing = OnRep_PlayerSlot,VisibleAnywhere, BlueprintReadOnly, Category="Player")
	int32 PlayerSlot = -1;
	
	void SetPlayerSlot(const int32 NewSlot);
	
	UFUNCTION()
	void OnRep_PlayerSlot();
	
#pragma region 자원
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Resource")
	int32 Wood = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Resource")
	int32 Ore = 0;
	
	void AddResource(const EResourceType ResourceType, const int32 Amount);
#pragma endregion	
	
#pragma region 병력 스폰
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Troop")
	int32 CurrentTroopCount = 0;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Troop")
	int32 PendingTroopCount = 0;
	
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category="Troop")
	int32 MaxTroopCount = 1;
	
	int8 CanAffordCost(const int32 InWoodCost, const int32 InOreCost) const;
	void SpendCost(const int32 InWoodCost, const int32 InOreCost);
	
	int8 CanQueueTroop(const int32 InAmount = 1) const;
	
	void AddTroopCount(const int32 InAmount);
	void RemoveTroopCount(const int32 InAmount);
	
	void AddPendingTroopCount(const int32 InAmount);
	void RemovePendingTroopCount(const int32 InAmount);
	
	void AddUnit(FMassEntityHandle& Unit );
	void RemoveUnit(int32 Idx);
	TArray<FMassEntityHandle> Units;
#pragma endregion
	
#pragma region 인구 수
	void AddPopulationCap(const int32 InAmount);
#pragma endregion
	
#pragma region 병력 유지비
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Troop|Upkeep")
	FBuildingResourceCost UpkeepCost;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Troop|Upkeep", meta=(ClampMin="0.1"))
	float UpkeepInterval = 60.0f;

	FTimerHandle TroopUpkeepTimerHandle;

	FBuildingResourceCost GetTotalTroopUpkeepCost() const;
	void RefreshTroopUpkeepTimer();
	void HandleTroopUpkeep();
	int8 TrySpendTroopUpkeep();
#pragma endregion
};
