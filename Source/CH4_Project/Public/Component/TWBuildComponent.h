#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TWBuildComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CH4_PROJECT_API UTWBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTWBuildComponent();

protected:

	virtual void BeginPlay() override;

public:

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
