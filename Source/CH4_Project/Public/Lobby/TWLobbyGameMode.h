// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TWLobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATWLobbyGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ATWLobbyGameMode();
	
	virtual void PreLogin(
		const FString& Options, 
		const FString& Address, 
		const FUniqueNetIdRepl& UniqueId, 
		FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	bool CheckStartCondition();
	void StartGame();
	
protected:
	void AssignNewHost();
	
private:
	int32 MinPlayersToStart = 2;
	bool bAllReady;
};


