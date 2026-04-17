#include "Building/TWPopulationBuilding.h"
#include "Data/TWPopulationBuildingDataAsset.h"
#include "Core/TWPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "GameFramework/GameStateBase.h"

ATWPopulationBuilding::ATWPopulationBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

const UTWPopulationBuildingDataAsset* ATWPopulationBuilding::GetPopulationBuildingData() const
{
	return Cast<UTWPopulationBuildingDataAsset>(BuildingData);
}

int32 ATWPopulationBuilding::RequestEnqueuePopulation()
{
	if (!HasAuthority())
	{
		return 0;
	}

	if (!OwningPlayerState)
	{
		return 0;
	}

	if (BuildingState != ETWBuildingState::Completed)
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
	ForceNetUpdate();
	
	UE_LOG(LogTemp, Warning, TEXT("[인구수 건물] 대기열 추가 | 현재 대기열 수: %d"), CurrentQueueCount);

	TryStartNextProduction();
	return 1;
}


int32 ATWPopulationBuilding::IncreasePopulationNow()
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

void ATWPopulationBuilding::TryStartNextProduction()
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (bIsProducing)
	{
		return;
	}

	if (CurrentQueueCount <= 0)
	{
		return;
	}

	const UTWPopulationBuildingDataAsset* PopulationData = GetPopulationBuildingData();
	if (!PopulationData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bIsProducing = true;
	CurrentProducingDuration = FMath::Max(0.01f, PopulationData->ProductionInterval);

	const AGameStateBase* GS = World->GetGameState();
	CurrentProductionStartTime = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();

	ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[인구수 건물] 생산 시작 | 생산 시간: %.2f초 | 현재 대기열 수: %d"),
		CurrentProducingDuration,
		CurrentQueueCount
	);

	GetWorldTimerManager().SetTimer(
		PopulationQueueTimerHandle,
		this,
		&ATWPopulationBuilding::HandlePopulationQueue,
		CurrentProducingDuration,
		false
	);
}

void ATWPopulationBuilding::HandlePopulationQueue()
{
	if (!HasAuthority())
	{
		return;
	}

	auto ResetProductionState = [this]()
	{
		bIsProducing = false;
		CurrentProducingDuration = 0.f;
		CurrentProductionStartTime = 0.f;
		ForceNetUpdate();
	};
	
	if (CurrentQueueCount <= 0)
	{
		ResetProductionState();
		return;
	}

	const int8 bIncreaseSuccess = IncreasePopulationNow();
	if (bIncreaseSuccess == 0)
	{
		ResetProductionState();
		return;
	}

	CurrentQueueCount = FMath::Max(0, CurrentQueueCount - 1);

	UE_LOG(LogTemp, Log, TEXT("[인구수 건물] 현재 대기열 수: %d"), CurrentQueueCount);

	ResetProductionState();
	TryStartNextProduction();
}

float ATWPopulationBuilding::GetCurrentProductionProgressRatio() const
{
	if (!bIsProducing || CurrentProducingDuration <= 0.f)
	{
		return 0.f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	const AGameStateBase* GS = World->GetGameState();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	const float Elapsed = FMath::Max(0.f, Now - CurrentProductionStartTime);

	return FMath::Clamp(Elapsed / CurrentProducingDuration, 0.f, 1.f);
}

FString ATWPopulationBuilding::GetCurrentProductionProgressText() const
{
	const float Ratio = GetCurrentProductionProgressRatio();
	return FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Ratio * 100.f));
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
	bIsProducing = false;
	CurrentProducingDuration = 0.f;
	CurrentProductionStartTime = 0.f;
	
	CancelQueuedPopulation();
	ForceNetUpdate();
}

void ATWPopulationBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWPopulationBuilding, CurrentQueueCount);
	DOREPLIFETIME(ATWPopulationBuilding, bIsProducing);
	DOREPLIFETIME(ATWPopulationBuilding, CurrentProducingDuration);
	DOREPLIFETIME(ATWPopulationBuilding, CurrentProductionStartTime);
}