#include "FOW/TWVisionComponent.h"
#include "FOW/TWFogManager.h"
#include "Kismet/GameplayStatics.h"

UTWVisionComponent::UTWVisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UTWVisionComponent::BeginPlay()
{
    Super::BeginPlay();

    ATWFogManager* FogManager = Cast<ATWFogManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), 
            ATWFogManager::StaticClass())
            );
    
    if (FogManager)
    {
        FogManager->RegisterVision(this);
    }
}

void UTWVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    ATWFogManager* FogManager = Cast<ATWFogManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), 
            ATWFogManager::StaticClass())
            );
    
    if (FogManager)
    {
        FogManager->UnregisterVision(this);
    }
}

FVector UTWVisionComponent::GetVisionLocation() const
{
    return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}
