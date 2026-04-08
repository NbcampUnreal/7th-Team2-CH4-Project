#include "FOW/TW_VisionComponent.h"
#include "FOW/TW_FogManager.h"
#include "Kismet/GameplayStatics.h"

UTW_VisionComponent::UTW_VisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UTW_VisionComponent::BeginPlay()
{
    Super::BeginPlay();

    ATW_FogManager* FogManager = Cast<ATW_FogManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), 
            ATW_FogManager::StaticClass())
            );
    
    if (FogManager)
    {
        FogManager->RegisterVision(this);
    }
}

void UTW_VisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    ATW_FogManager* FogManager = Cast<ATW_FogManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), 
            ATW_FogManager::StaticClass())
            );
    
    if (FogManager)
    {
        FogManager->UnregisterVision(this);
    }
}

FVector UTW_VisionComponent::GetVisionLocation() const
{
    return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}
