// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyGameState.h"
#include "Net/UnrealNetwork.h"  // DOREPLIFETIME 매크로용

ATWLobbyGameState::ATWLobbyGameState()
{
	
}


void ATWLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

