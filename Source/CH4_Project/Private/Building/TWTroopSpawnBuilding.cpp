#include "Building/TWTroopSpawnBuilding.h"
#include "Data/TWTroopBuildingDataAsset.h"
#include "Data/TWUnitTableRowBase.h"
#include "Components/SceneComponent.h"
#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "TimerManager.h"

ATWTroopSpawnBuilding::ATWTroopSpawnBuilding()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));

    if (GetRootComponent())
    {
        SpawnPoint->SetupAttachment(GetRootComponent());
    }
    else
    {
        RootComponent = SpawnPoint;
    }
}

const UTWTroopBuildingDataAsset* ATWTroopSpawnBuilding::GetTroopBuildingData() const
{
    return Cast<UTWTroopBuildingDataAsset>(BuildingData);
}

void ATWTroopSpawnBuilding::RequestEnqueueTroop(const FName InUnitID)
{
    if (!HasAuthority())
    {
        return;
    }

    const UTWTroopBuildingDataAsset* TroopData = GetTroopBuildingData();
    if (!TroopData)
    {
        return;
    }

    if (!OwningPlayerState)
    {
        return;
    }
    
    if (QueuePausedByUpkeep == 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("유지비 부족으로 건물 pause 상태 - 병력 대기열 추가 불가"));
        return;
    }

    UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
    if (!UnitSubsystem)
    {
        return;
    }

    const FName RequestUnitID = InUnitID.IsNone() ? TroopData->DefaultUnitID : InUnitID;
    if (RequestUnitID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("생산할 UnitID가 설정되지 않음"));
		return;
	}

    if (TroopData->TrainableUnitIDs.Num() > 0 && !TroopData->TrainableUnitIDs.Contains(RequestUnitID))
    {
        UE_LOG(LogTemp, Warning, TEXT("이 건물은 해당 유닛을 생산할 수 없음: %s"), *RequestUnitID.ToString());
        return;
    }

    FTWUnitTableRowBase* UnitRow = UnitSubsystem->GetUnitTableRowBase(RequestUnitID);
    if (!UnitRow)
    {
        UE_LOG(LogTemp, Warning, TEXT("유닛 데이터 테이블에서 UnitID를 찾지 못함: %s"), *RequestUnitID.ToString());
        return;
    }

    if (CurrentQueueCount >= TroopData->MaxQueueCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("병력 대기열 가득 참"));
        return;
    }

    if (OwningPlayerState->CanQueueTroop(UnitRow->Population) == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("인구수 부족 - 병력 대기열 추가 불가"));
        return;
    }

    if (OwningPlayerState->CanAffordCost(UnitRow->Cost) == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("병력 대기열 추가 실패: 자원 부족"));
        return;
    }

    OwningPlayerState->SpendCost(UnitRow->Cost);
    OwningPlayerState->AddPendingPopulation(UnitRow->Population);

    FTWQueuedTroopData NewQueueData;
    NewQueueData.UnitID = RequestUnitID;
    TroopQueue.Add(NewQueueData);
    
    CurrentQueueCount = TroopQueue.Num();

    UE_LOG(LogTemp, Log, TEXT("병력 대기열 추가: %s | 현재 대기열 수: %d"),
        *RequestUnitID.ToString(),
        CurrentQueueCount);

    StartSpawnQueueTimer();
}

int8 ATWTroopSpawnBuilding::SpawnUnitNow()
{
    if (!HasAuthority())
    {
        return 0;
    }

    if (!OwningPlayerState)
    {
        return 0;
    }

    if (TroopQueue.IsEmpty())
    {
        return 0;
    }

    UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
    if (!UnitSubsystem)
    {
        return 0;
    }

    const FTWQueuedTroopData& QueueData = TroopQueue[0];
    FTWUnitTableRowBase* UnitRow = UnitSubsystem->GetUnitTableRowBase(QueueData.UnitID);
    if (!UnitRow)
    {
        return 0;
    }
    
    ATWPlayerController* OwningPlayerController = nullptr;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        ATWPlayerController* PC = Cast<ATWPlayerController>(It->Get());
        if (!PC)
        {
            continue;
        }

        if (PC->GetPlayerState<ATWPlayerState>() == OwningPlayerState)
        {
            OwningPlayerController = PC;
            break;
        }
    }

    if (!OwningPlayerController)
    {
        return 0;
    }
    
    const FVector SpawnLocation = SpawnPoint
        ? SpawnPoint->GetComponentLocation()
        : GetActorLocation() + GetActorForwardVector() * 150.0f;

    UnitSubsystem->SpawnUnit(SpawnLocation, *UnitRow, OwningPlayerController);
    return 1;
}

void ATWTroopSpawnBuilding::SetQueuePausedByUpkeep(const uint8 bInPaused)
{
    if (!HasAuthority())
    {
        return;
    }

    QueuePausedByUpkeep = bInPaused;
}

void ATWTroopSpawnBuilding::StartSpawnQueueTimer()
{
    if (!HasAuthority())
    {
        return;
    }

    if (GetWorldTimerManager().IsTimerActive(SpawnQueueTimerHandle))
    {
        return;
    }

    if (TroopQueue.IsEmpty())
    {
        return;
    }

    UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
    if (!UnitSubsystem)
    {
        return;
    }

    FTWUnitTableRowBase* UnitRow = UnitSubsystem->GetUnitTableRowBase(TroopQueue[0].UnitID);
    if (!UnitRow)
    {
        return;
    }

    if (UnitRow->SpawnDuration <= 0.0f)
    {
        return;
    }

    GetWorldTimerManager().SetTimer(
        SpawnQueueTimerHandle,
        this,
        &ATWTroopSpawnBuilding::HandleSpawnQueue,
        UnitRow->SpawnDuration,
        false
    );
}

void ATWTroopSpawnBuilding::HandleSpawnQueue()
{
    if (!HasAuthority())
    {
        return;
    }

    if (TroopQueue.IsEmpty())
    {
        CurrentQueueCount = 0;
        ClearAllBuildingTimers();
        return;
    }

    if (QueuePausedByUpkeep == 1)
    {
        GetWorldTimerManager().SetTimer(
            SpawnQueueTimerHandle,
            this,
            &ATWTroopSpawnBuilding::HandleSpawnQueue,
            0.2f,
            false
        );
        return;
    }

    const int8 bSpawnSuccess = SpawnUnitNow();
    if (bSpawnSuccess == 0)
    {
        return;
    }

    TroopQueue.RemoveAt(0);
    CurrentQueueCount = TroopQueue.Num();

    UE_LOG(LogTemp, Log, TEXT("현재 병력 대기열 수: %d"), CurrentQueueCount);

    if (CurrentQueueCount <= 0)
    {
        ClearAllBuildingTimers();
        return;
    }

    StartSpawnQueueTimer();
}

void ATWTroopSpawnBuilding::ClearAllBuildingTimers()
{
    if (!HasAuthority())
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(SpawnQueueTimerHandle);
}

void ATWTroopSpawnBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATWTroopSpawnBuilding, CurrentQueueCount);
    DOREPLIFETIME(ATWTroopSpawnBuilding, QueuePausedByUpkeep);
}