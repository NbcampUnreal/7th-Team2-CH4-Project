// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TWLobbyGameState.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATWLobbyGameState : public AGameState
{
	GENERATED_BODY()
	
public:
	ATWLobbyGameState();
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
