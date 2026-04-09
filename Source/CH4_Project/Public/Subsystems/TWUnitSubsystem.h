// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityQuery.h"
#include "Subsystems/WorldSubsystem.h"
#include "TWUnitSubsystem.generated.h"

/**
 * 
 */

struct FMassEntityHandle;

UCLASS()
class CH4_PROJECT_API UTWUnitSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void PostInitialize() override;
	
	bool FindNearestEntity(const FVector& Location, FMassEntityHandle& OutEntityHandle, float MaxDistance = 50.0f);
	bool GetAllEntities(const FVector& StartLocation, const FVector& EndLocation, TArray<FMassEntityHandle>& OutEntityHandles);

private:
	FMassEntityQuery FindNearestEntityQuery;
	
};
