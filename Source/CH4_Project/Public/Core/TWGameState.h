// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TWGameState.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATWGameState : public AGameState
{
	GENERATED_BODY()
public:
	FORCEINLINE float GetUpkeepRatio()const{return UpkeepRatio;}
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float UpkeepRatio;
};
