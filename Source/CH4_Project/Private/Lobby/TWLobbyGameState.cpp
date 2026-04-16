// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyGameState.h"
#include "Net/UnrealNetwork.h"  // DOREPLIFETIME 매크로용

ATWLobbyGameState::ATWLobbyGameState()
{
	bIsCanStartGame = false;
}

void ATWLobbyGameState::SetCanStartGame(bool bInCanStart)
{
	bIsCanStartGame = bInCanStart;
}

void ATWLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATWLobbyGameState, bIsCanStartGame)
}

void ATWLobbyGameState::OnRep_CanStartGame()
{
	UE_LOG(LogTemp, Log, TEXT("GameState: CanStartGame changed to %s"), bIsCanStartGame ? TEXT("True") : TEXT("False"));
}
