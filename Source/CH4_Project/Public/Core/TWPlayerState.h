#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "Core/TWPlayerUnitContainer.h"
#include "MassEntityHandle.h"
#include "GameFramework/PlayerState.h"
#include "Data/TWBuildingTypes.h"
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
	// 0 = Player1, 1 = Player2
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Player")
	int32 PlayerSlot = -1;
	
	void SetPlayerSlot(const int32 InPlayerSlot);
	
#pragma region 자원
protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Resource")
	TArray<int32> Resources;
public:
	void AddResource(const EResourceType ResourceType, const int32 Amount);
	
#pragma endregion	
	
#pragma region 병력 스폰
protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Troop")
	int32 PendingTroopCount = 0;
	
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category="Troop")
	int32 MaxTroopCount = 1;
public:
	int8 CanAffordCost(const TMap<EResourceType, int32>& Cost) const;
	void SpendCost(const TMap<EResourceType, int32>& Cost);
	
	int8 CanQueueTroop(const int32 InAmount = 1) const;
	
	void AddTroopCount(const int32 InAmount);
	void RemoveTroopCount(const int32 InAmount);
	
	void AddPendingTroopCount(const int32 InAmount);
	void RemovePendingTroopCount(const int32 InAmount);
	
	int32 CurrentTroopCount=10;

#pragma endregion
	
#pragma region 인구 수
	void AddPopulationCap(const int32 InAmount);
#pragma endregion
	
#pragma region 병력 유지비
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Troop|Upkeep")
	TMap<EResourceType, int32> UpkeepCost;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Troop|Upkeep", meta=(ClampMin="0.1"))
	float UpkeepInterval = 60.0f;

	FTimerHandle TroopUpkeepTimerHandle;
public:
	FORCEINLINE const TMap<EResourceType, int32>& GetTotalTroopUpkeepCost() const{return UpkeepCost;}
	void SetTotalTroopUpkeepCost(const TMap<EResourceType, int32>& Upkeep){UpkeepCost = Upkeep;}
	void RefreshTroopUpkeepTimer();
	void HandleTroopUpkeep();
	int8 TrySpendTroopUpkeep();
#pragma endregion
	
private:
	FORCEINLINE UTWUnitSubsystem* GetUnitSubsystem() const
	{
		return GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;
	}
};