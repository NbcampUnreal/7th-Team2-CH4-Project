// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "TWCheatManager.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API UTWCheatManager : public UCheatManager
{
	GENERATED_BODY()
	
public:
	UFUNCTION(Exec)
	void AddResource(FString ResourceType, int32 Amount);
	
	UFUNCTION(Exec)
	void TimeScale(float TimeMultiplier);
};
