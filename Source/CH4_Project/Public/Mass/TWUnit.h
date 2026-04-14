// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassAgentComponent.h"
#include "TWUnit.generated.h"

UCLASS()
class CH4_PROJECT_API ATWUnit : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATWUnit();

protected:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category=Component)
	TObjectPtr<USceneComponent> SceneComponent;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category=Component)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category=Component)
	TObjectPtr<UMassAgentComponent> MassAgentComponent;
};
