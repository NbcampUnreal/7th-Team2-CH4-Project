#include "Component/TWBuildComponent.h"


UTWBuildComponent::UTWBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UTWBuildComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UTWBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

