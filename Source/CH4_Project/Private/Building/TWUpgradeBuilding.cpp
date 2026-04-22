#include "Building/TWUpgradeBuilding.h"

#include "Core/TWPlayerState.h"
#include "Data/TWUpgradeTableRowBase.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Log/TWLogCategory.h"

ATWUpgradeBuilding::ATWUpgradeBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

int8 ATWUpgradeBuilding::RequestStartUpgrade(const FName InUpgradeID)
{
	if (!HasAuthority())
	{
		return 0;
	}
	
	if (BuildingState != ETWBuildingState::Completed)
	{
		return 0;
	}

	if (!OwningPlayerState)
	{
		return 0;
	}

	if (InUpgradeID.IsNone())
	{
		return 0;
	}

	if (MaxUpgradeQueueCount > 0 && UpgradeQueue.Num() >= MaxUpgradeQueueCount)
	{
		return 0;
	}

	FTWUpgradeTableRowBase* UpgradeRow = GetUpgradeRow(InUpgradeID);
	if (!UpgradeRow)
	{
		return 0;
	}

	const TMap<EResourceType, int32> FinalCost = BuildCurrentUpgradeCost(*UpgradeRow);
	if (OwningPlayerState->CanAffordCost(FinalCost) == 0)
	{
		UE_LOG(
			LogTWProduction,
			Warning,
			TEXT("[업그레이드 건물] 업그레이드 요청 실패: 자원 부족 | UpgradeID: %s"),
			*InUpgradeID.ToString()
		);
		return 0;
	}

	OwningPlayerState->SpendCost(FinalCost);

	UpgradeQueue.Add(UpgradeRow->UpgradeID.IsNone() ? InUpgradeID : UpgradeRow->UpgradeID);
	ForceNetUpdate();
	TryStartNextUpgrade();
	return 1;
}

FTWUpgradeTableRowBase* ATWUpgradeBuilding::GetUpgradeRow(const FName InUpgradeID) const
{
	if (!UpgradeTable || InUpgradeID.IsNone())
	{
		return nullptr;
	}

	// 1차: RowName으로 직접 조회
	if (FTWUpgradeTableRowBase* DirectRow = UpgradeTable->FindRow<FTWUpgradeTableRowBase>(
		InUpgradeID,
		TEXT("ATWUpgradeBuilding::GetUpgradeRow_Direct")))
	{
		return DirectRow;
	}

	// 2차: 테이블 전체 순회 후 UpgradeID 필드로 조회
	static const FString ContextString(TEXT("ATWUpgradeBuilding::GetUpgradeRow_SearchByUpgradeID"));

	TArray<FTWUpgradeTableRowBase*> AllRows;
	UpgradeTable->GetAllRows<FTWUpgradeTableRowBase>(ContextString, AllRows);

	for (FTWUpgradeTableRowBase* Row : AllRows)
	{
		if (!Row)
		{
			continue;
		}

		if (Row->UpgradeID == InUpgradeID)
		{
			return Row;
		}
	}

	return nullptr;
}

TMap<EResourceType, int32> ATWUpgradeBuilding::BuildCurrentUpgradeCost(const FTWUpgradeTableRowBase& UpgradeRow) const
{
	TMap<EResourceType, int32> FinalCost = UpgradeRow.Cost;

	if (!OwningPlayerState)
	{
		return FinalCost;
	}

	const FName EffectiveUpgradeID = UpgradeRow.UpgradeID.IsNone() ? NAME_None : UpgradeRow.UpgradeID;
	const int32 CurrentLevel = OwningPlayerState->GetUpgradeLevelByID(EffectiveUpgradeID);
	const int32 PendingSameUpgradeCount = CountQueuedSameUpgrade(EffectiveUpgradeID);
	const int32 EffectiveLevel = CurrentLevel + PendingSameUpgradeCount;

	for (const TPair<EResourceType, int32>& Pair : UpgradeRow.CostIncreaseAmount)
	{
		FinalCost.FindOrAdd(Pair.Key) += Pair.Value * EffectiveLevel;
	}

	return FinalCost;
}

int32 ATWUpgradeBuilding::CountQueuedSameUpgrade(const FName InUpgradeID) const
{
	if (InUpgradeID.IsNone())
	{
		return 0;
	}

	int32 Count = 0;

	for (const FName& QueuedUpgradeID : UpgradeQueue)
	{
		if (QueuedUpgradeID == InUpgradeID)
		{
			++Count;
		}
	}

	return Count;
}

void ATWUpgradeBuilding::TryStartNextUpgrade()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsUpgradeInProgress != 0)
	{
		return;
	}

	if (UpgradeQueue.Num() <= 0)
	{
		return;
	}

	const FName NextUpgradeID = UpgradeQueue[0];
	FTWUpgradeTableRowBase* UpgradeRow = GetUpgradeRow(NextUpgradeID);

	if (!UpgradeRow)
	{
		UpgradeQueue.RemoveAt(0);
		ForceNetUpdate();

		TryStartNextUpgrade();
		return;
	}

	StartUpgradeInternal(NextUpgradeID, *UpgradeRow);
}

void ATWUpgradeBuilding::StartUpgradeInternal(const FName InUpgradeID, const FTWUpgradeTableRowBase& UpgradeRow)
{
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FName EffectiveUpgradeID = UpgradeRow.UpgradeID.IsNone() ? InUpgradeID : UpgradeRow.UpgradeID;

	bIsUpgradeInProgress = 1;
	CurrentUpgradeID = EffectiveUpgradeID;
	CurrentUpgradeDuration = FMath::Max(0.01f, UpgradeRow.Duration);

	if (const AGameStateBase* GameState = World->GetGameState())
	{
		CurrentUpgradeStartTime = GameState->GetServerWorldTimeSeconds();
	}
	else
	{
		CurrentUpgradeStartTime = World->GetTimeSeconds();
	}

	ForceNetUpdate();

	GetWorldTimerManager().SetTimer(
		UpgradeTimerHandle,
		this,
		&ATWUpgradeBuilding::FinishUpgrade,
		CurrentUpgradeDuration,
		false
	);
}

void ATWUpgradeBuilding::FinishUpgrade()
{
	if (!HasAuthority())
	{
		return;
	}

	auto ResetUpgradeState = [this]()
	{
		bIsUpgradeInProgress = 0;
		CurrentUpgradeID = NAME_None;
		CurrentUpgradeDuration = 0.f;
		CurrentUpgradeStartTime = 0.f;
		ForceNetUpdate();
	};

	if (UpgradeQueue.Num() <= 0)
	{
		ResetUpgradeState();
		return;
	}

	if (!OwningPlayerState)
	{
		UpgradeQueue.RemoveAt(0);
		ResetUpgradeState();
		TryStartNextUpgrade();
		return;
	}

	const FName FinishedUpgradeID = UpgradeQueue[0];
	FTWUpgradeTableRowBase* UpgradeRow = GetUpgradeRow(FinishedUpgradeID);

	if (!UpgradeRow)
	{
		UpgradeQueue.RemoveAt(0);
		ResetUpgradeState();
		TryStartNextUpgrade();
		return;
	}

	OwningPlayerState->ApplyUpgradeRow(*UpgradeRow);
	UpgradeQueue.RemoveAt(0);

	ResetUpgradeState();
	TryStartNextUpgrade();
}

float ATWUpgradeBuilding::GetCurrentProductionProgressRatio() const
{
	if (bIsUpgradeInProgress == 0)
	{
		return 0.f;
	}

	if (CurrentUpgradeID.IsNone() || CurrentUpgradeDuration <= 0.f)
	{
		return 0.f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	float CurrentServerTime = 0.f;

	if (const AGameStateBase* GameState = World->GetGameState())
	{
		CurrentServerTime = GameState->GetServerWorldTimeSeconds();
	}
	else
	{
		CurrentServerTime = World->GetTimeSeconds();
	}

	const float ElapsedTime = FMath::Max(0.f, CurrentServerTime - CurrentUpgradeStartTime);
	return FMath::Clamp(ElapsedTime / CurrentUpgradeDuration, 0.f, 1.f);
}

FString ATWUpgradeBuilding::GetCurrentProductionProgressText() const
{
	if (bIsUpgradeInProgress == 0)
	{
		return TEXT("0%");
	}

	const float Ratio = GetCurrentProductionProgressRatio();
	const int32 Percent = FMath::Clamp(FMath::RoundToInt(Ratio * 100.f), 0, 100);

	return FString::Printf(TEXT("%d%%"), Percent);
}

bool ATWUpgradeBuilding::ResolveAvailableUpgradeIds(TArray<FName>& OutUpgradeIds) const
{
	OutUpgradeIds.Reset();

	if (!UpgradeTable)
	{
		return false;
	}

	static const FString ContextString(TEXT("ATWUpgradeBuilding::ResolveAvailableUpgradeIds"));

	TArray<FTWUpgradeTableRowBase*> AllRows;
	UpgradeTable->GetAllRows<FTWUpgradeTableRowBase>(ContextString, AllRows);

	for (const FTWUpgradeTableRowBase* Row : AllRows)
	{
		if (!Row)
		{
			continue;
		}

		if (!Row->UpgradeID.IsNone())
		{
			OutUpgradeIds.Add(Row->UpgradeID);
		}
	}

	OutUpgradeIds.Remove(NAME_None);
	return OutUpgradeIds.Num() > 0;
}

void ATWUpgradeBuilding::ClearAllBuildingTimers()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(UpgradeTimerHandle);

	UpgradeQueue.Empty();
	bIsUpgradeInProgress = 0;
	CurrentUpgradeID = NAME_None;
	CurrentUpgradeDuration = 0.f;
	CurrentUpgradeStartTime = 0.f;

	ForceNetUpdate();
}

void ATWUpgradeBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWUpgradeBuilding, bIsUpgradeInProgress);
	DOREPLIFETIME(ATWUpgradeBuilding, CurrentUpgradeID);
	DOREPLIFETIME(ATWUpgradeBuilding, UpgradeQueue);
	DOREPLIFETIME(ATWUpgradeBuilding, CurrentUpgradeDuration);
	DOREPLIFETIME(ATWUpgradeBuilding, CurrentUpgradeStartTime);
}