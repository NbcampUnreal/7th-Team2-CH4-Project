#include "Building/TWBlockingBuilding.h"
#include "Data/TWBlockingBuildingDataAsset.h"
#include "Net/UnrealNetwork.h"

void ATWBlockingBuilding::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    const UTWBlockingBuildingDataAsset* BlockingData = GetBlockingBuildingData();
    if (!BlockingData)
    {
        return;
    }

    MaxHP = FMath::Max(1, BlockingData->MaxHP);
    CurrentHP = MaxHP;
}

const UTWBlockingBuildingDataAsset* ATWBlockingBuilding::GetBlockingBuildingData() const
{
    return Cast<UTWBlockingBuildingDataAsset>(BuildingData);
}

void ATWBlockingBuilding::ApplyDamageToBuilding(const int32 InDamageAmount)
{
    if (!HasAuthority())
    {
        return;
    }

    if (InDamageAmount <= 0)
    {
        return;
    }

    if (CurrentHP <= 0)
    {
        return;
    }

    CurrentHP -= InDamageAmount;
    CurrentHP = FMath::Max(0, CurrentHP);

    UE_LOG(LogTemp, Warning, TEXT("[%s] HP : %d / %d"), *GetName(), CurrentHP, MaxHP);

    if (CurrentHP == 0)
    {
        HandleDestroyedByDamage();
    }
}

void ATWBlockingBuilding::HandleDestroyedByDamage()
{
    if (!HasAuthority())
    {
        return;
    }
    
    Destroy();
}

void ATWBlockingBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ATWBlockingBuilding, MaxHP);
    DOREPLIFETIME(ATWBlockingBuilding, CurrentHP);
}