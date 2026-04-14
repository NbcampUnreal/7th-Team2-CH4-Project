#include "Building/TWPopulationBuilding.h"
#include "Data/TWPopulationBuildingDataAsset.h"
#include "Core/TWPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

ATWPopulationBuilding::ATWPopulationBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

const UTWPopulationBuildingDataAsset* ATWPopulationBuilding::GetPopulationBuildingData() const
{
	return Cast<UTWPopulationBuildingDataAsset>(BuildingData);
}

int8 ATWPopulationBuilding::RequestEnqueuePopulation()
{
	if (!HasAuthority())
	{
		return 0;
	}

	if (!OwningPlayerState)
	{
		return 0;
	}

	const UTWPopulationBuildingDataAsset* PopulationData = GetPopulationBuildingData();
	if (!PopulationData)
	{
		return 0;
	}

	if (CurrentQueueCount >= PopulationData->MaxQueueCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[인구수 건물] 대기열 가득 참"));
		return 0;
	}

	if (OwningPlayerState->CanAffordCost(PopulationData->ProductionCost) == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[인구수 건물] 대기열 추가 실패: 자원 부족"));
		return 0;
	}

	OwningPlayerState->SpendCost(PopulationData->ProductionCost);

	CurrentQueueCount += 1;
	
	UE_LOG(LogTemp, Warning, TEXT("[인구수 건물] 대기열 추가 | 현재 대기열 수: %d"), CurrentQueueCount);

	StartPopulationQueueTimer();
	return 1;
}


int8 ATWPopulationBuilding::IncreasePopulationNow()
{
	if (!HasAuthority())
	{
		return 0;
	}

	if (!OwningPlayerState)
	{
		return 0;
	}

	const UTWPopulationBuildingDataAsset* PopulationData = GetPopulationBuildingData();
	if (!PopulationData)
	{
		return 0;
	}

	OwningPlayerState->AddPopulationLimit(1);
	return 1;
}

void ATWPopulationBuilding::StartPopulationQueueTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	const UTWPopulationBuildingDataAsset* PopulationData = GetPopulationBuildingData();
	if (!PopulationData)
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(PopulationQueueTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		PopulationQueueTimerHandle,
		this,
		&ATWPopulationBuilding::HandlePopulationQueue,
		PopulationData->ProductionInterval,
		true
	);
}

void ATWPopulationBuilding::HandlePopulationQueue()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentQueueCount <= 0)
	{
		ClearAllBuildingTimers();
		return;
	}

	const int8 bIncreaseSuccess = IncreasePopulationNow();
	if (bIncreaseSuccess == 0)
	{
		return;
	}

	CurrentQueueCount = FMath::Max(0, CurrentQueueCount - 1);

	UE_LOG(LogTemp, Log, TEXT("[인구수 건물] 현재 대기열 수: %d"), CurrentQueueCount);

	if (CurrentQueueCount <= 0)
	{
		ClearAllBuildingTimers();
	}
}

void ATWPopulationBuilding::CancelQueuedPopulation()
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentQueueCount = 0;
}

void ATWPopulationBuilding::ClearAllBuildingTimers()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PopulationQueueTimerHandle);
	CancelQueuedPopulation();
	ForceNetUpdate();
}

void ATWPopulationBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWPopulationBuilding, CurrentQueueCount);
}