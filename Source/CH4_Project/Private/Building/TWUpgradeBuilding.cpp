#include "Building/TWUpgradeBuilding.h"
#include "Core/TWPlayerState.h"
#include "Data/TWUpgradeTableRowBase.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

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

	if (!OwningPlayerState)
	{
		return 0;
	}

	if (bIsUpgradeInProgress == 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("이미 업그레이드 진행 중"));
		return 0;
	}
	
	if (InUpgradeID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("업그레이드 ID가 설정되지 않음"));
		return 0;
	}

	FTWUpgradeTableRowBase* UpgradeRow = GetUpgradeRow(InUpgradeID);
	if (!UpgradeRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("업그레이드 데이터 없음: %s"), *InUpgradeID.ToString());
		return 0;
	}

	const TMap<EResourceType, int32> FinalCost = BuildCurrentUpgradeCost(*UpgradeRow);

	if (OwningPlayerState->CanAffordCost(FinalCost) == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("업그레이드 시작 실패: 자원 부족"));
		return 0;
	}

	OwningPlayerState->SpendCost(FinalCost);

	bIsUpgradeInProgress = 1;
	CurrentUpgradeID = InUpgradeID;

	const float UpgradeDuration = FMath::Max(0.01f, UpgradeRow->Duration);

	GetWorldTimerManager().SetTimer(
		UpgradeTimerHandle,
		this,
		&ATWUpgradeBuilding::FinishUpgrade,
		UpgradeDuration,
		false
	);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("업그레이드 시작 | UpgradeID: %s | StatusType: %s | TargetUnitCount: %d"),
		*CurrentUpgradeID.ToString(),
		*StaticEnum<ETWStatusType>()->GetNameStringByValue(static_cast<int64>(UpgradeRow->TargetStatus)),
		UpgradeRow->TargetUnits.Num()
	);

	return 1;
}

void ATWUpgradeBuilding::FinishUpgrade()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!OwningPlayerState)
	{
		bIsUpgradeInProgress = 0;
		CurrentUpgradeID = NAME_None;
		return;
	}

	FTWUpgradeTableRowBase* UpgradeRow = GetUpgradeRow(CurrentUpgradeID);
	if (!UpgradeRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("업그레이드 완료 실패: 데이터 없음 | UpgradeID: %s"), *CurrentUpgradeID.ToString());

		bIsUpgradeInProgress = 0;
		CurrentUpgradeID = NAME_None;
		return;
	}

	OwningPlayerState->ApplyUpgradeRow(*UpgradeRow);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("업그레이드 완료 | UpgradeID: %s | StatusType: %s | Level: %d"),
		*CurrentUpgradeID.ToString(),
		*StaticEnum<ETWStatusType>()->GetNameStringByValue(static_cast<int64>(UpgradeRow->TargetStatus)),
		OwningPlayerState->GetUpgradeLevelByID(CurrentUpgradeID)
	);

	bIsUpgradeInProgress = 0;
	CurrentUpgradeID = NAME_None;
}

FTWUpgradeTableRowBase* ATWUpgradeBuilding::GetUpgradeRow(const FName InUpgradeID) const
{
	if (!UpgradeTable || InUpgradeID.IsNone())
	{
		return nullptr;
	}

	return UpgradeTable->FindRow<FTWUpgradeTableRowBase>(
		InUpgradeID,
		TEXT("ATWUpgradeBuilding::GetUpgradeRow")
	);
}

TMap<EResourceType, int32> ATWUpgradeBuilding::BuildCurrentUpgradeCost(const FTWUpgradeTableRowBase& UpgradeRow) const
{
	TMap<EResourceType, int32> FinalCost = UpgradeRow.Cost;

	if (!OwningPlayerState)
	{
		return FinalCost;
	}

	const int32 CurrentLevel = OwningPlayerState->GetUpgradeLevelByID(UpgradeRow.UpgradeID);

	for (const TPair<EResourceType, int32>& Pair : UpgradeRow.CostIncreaseAmount)
	{
		FinalCost.FindOrAdd(Pair.Key) += Pair.Value * CurrentLevel;
	}

	return FinalCost;
}

void ATWUpgradeBuilding::ClearAllBuildingTimers()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(UpgradeTimerHandle);
}

void ATWUpgradeBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWUpgradeBuilding, bIsUpgradeInProgress);
	DOREPLIFETIME(ATWUpgradeBuilding, CurrentUpgradeID);
}