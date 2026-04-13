#include "Building/TWResourceBuilding.h"
#include "Data/TWResourceBuildingDataAsset.h"
#include "Core/TWPlayerState.h"
#include "TimerManager.h"
#include "Engine/Engine.h"

const UTWResourceBuildingDataAsset* ATWResourceBuilding::GetResourceBuildingData() const
{
    return Cast<UTWResourceBuildingDataAsset>(BuildingData);
}

void ATWResourceBuilding::OnOwnerPlayerStateAssigned()
{
    if (!HasAuthority())
    {
        return;
    }

    StartResourceProduction();
}

void ATWResourceBuilding::StartResourceProduction()
{
    if (!HasAuthority())
    {
        return;
    }

    const UTWResourceBuildingDataAsset* ResourceData = GetResourceBuildingData();
    if (!ResourceData || !OwningPlayerState)
    {
        return;
    }

    if (ResourceData->ProducedResourceType == EResourceType::None)
    {
        return;
    }

    if (ResourceData->ProductionInterval <= 0.0f)
    {
        return;
    }

    ClearAllBuildingTimers();

    GetWorldTimerManager().SetTimer(
        ResourceProductionTimerHandle,
        this,
        &ATWResourceBuilding::HandleProduceResource,
        ResourceData->ProductionInterval,
        true
    );
}

void ATWResourceBuilding::HandleProduceResource()
{
    if (!HasAuthority())
    {
        return;
    }

    const UTWResourceBuildingDataAsset* ResourceData = GetResourceBuildingData();
    if (!ResourceData || !OwningPlayerState)
    {
        return;
    }

    OwningPlayerState->AddResource(
        ResourceData->ProducedResourceType,
        ResourceData->ProductionAmount
    );


}

void ATWResourceBuilding::ClearAllBuildingTimers()
{
    if (!HasAuthority())
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(ResourceProductionTimerHandle);
}