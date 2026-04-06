// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnitSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API UUnitSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void PostInitialize() override;
};
