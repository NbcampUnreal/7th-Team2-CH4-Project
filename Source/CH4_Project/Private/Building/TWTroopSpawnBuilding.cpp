#include "Building/TWTroopSpawnBuilding.h"
#include "Data/TWTroopBuildingDataAsset.h"
#include "Components/SceneComponent.h"
#include "Core/TWPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
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

void ATWTroopSpawnBuilding::RequestEnqueueTroop()
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

    if (!TroopData->SpawnActorClass)
    {
        return;
    }

    if (TroopData->SpawnInterval <= 0.0f)
    {
        return;
    }

    if (CurrentQueueCount >= TroopData->MaxQueueCount)
    {
        UE_LOG(LogTemp, Warning, TEXT("병력 대기열 가득 참"));
        return;
    }

    if (OwningPlayerState->CanQueueTroop(1) == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("최대 병력 수 보유(대기열 추가 X) - 최대 인구수"));
        return;
    }
    
    if (OwningPlayerState->CanAffordCost(TroopData->SpawnCost) == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("병력 대기열 추가 실패: 자원 부족"));
        return;
    }
    
    OwningPlayerState->SpendCost(TroopData->SpawnCost);

    CurrentQueueCount += 1;
    OwningPlayerState->AddPendingTroopCount(1);

    UE_LOG(LogTemp, Warning, TEXT("병력 대기열 추가: %d"), CurrentQueueCount);
    StartSpawnQueueTimer();
}

int8 ATWTroopSpawnBuilding::SpawnUnitNow()
{
    if (!HasAuthority())
    {
        return 0;
    }

    const UTWTroopBuildingDataAsset* TroopData = GetTroopBuildingData();
    if (!TroopData)
    {
        return 0;
    }

    if (!TroopData->SpawnActorClass)
    {
        return 0;
    }
    
    if (!OwningPlayerState)
    {
        return 0;
    }

    const FVector SpawnLocation = SpawnPoint
        ? SpawnPoint->GetComponentLocation()
        : GetActorLocation() + GetActorForwardVector() * 150.0f;

    const FRotator SpawnRotation = SpawnPoint
        ? SpawnPoint->GetComponentRotation()
        : GetActorRotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* SpawnedActor = GetWorld()->SpawnActor<AActor>(
        TroopData->SpawnActorClass,
        SpawnLocation,
        SpawnRotation,
        SpawnParams
    );
    
    if (!SpawnedActor)
    {
        return 0;
    }
    
    OwningPlayerState->AddTroopCount(1);
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

    const UTWTroopBuildingDataAsset* TroopData = GetTroopBuildingData();
    if (!TroopData)
    {
        return;
    }

    if (GetWorldTimerManager().IsTimerActive(SpawnQueueTimerHandle))
    {
        return;
    }

    GetWorldTimerManager().SetTimer(
        SpawnQueueTimerHandle,
        this,
        &ATWTroopSpawnBuilding::HandleSpawnQueue,
        TroopData->SpawnInterval,
        true
    );
}

void ATWTroopSpawnBuilding::HandleSpawnQueue()
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
    
    if (QueuePausedByUpkeep == 1)
    {
        return;
    }

    const int8 bSpawnSuccess = SpawnUnitNow();
    if (bSpawnSuccess == 0)
    {
        return;
    }

    CurrentQueueCount = FMath::Max(0, CurrentQueueCount - 1);

    if (OwningPlayerState)
    {
        OwningPlayerState->RemovePendingTroopCount(1);
    }

    UE_LOG(LogTemp, Log, TEXT("현재 병력 대기열 수: %d"), CurrentQueueCount);

    if (CurrentQueueCount <= 0)
    {
        ClearAllBuildingTimers();
    }
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