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
	
	void SetCurrentPlayerCount(int32 InCurrentPlayerCount);
	
	FORCEINLINE int32 GetCurrentPlayerCount() const { return CurrentPlayerCount; }
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
		
	UPROPERTY(ReplicatedUsing = OnRep_CurrentPlayerCount)
	int32 CurrentPlayerCount;
		
	UFUNCTION()
	void OnRep_CurrentPlayerCount();
};
