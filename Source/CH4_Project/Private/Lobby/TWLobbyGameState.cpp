// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/TWLobbyGameState.h"

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobby_Layout.h"
#include "Net/UnrealNetwork.h"

ATWLobbyGameState::ATWLobbyGameState()
{
	CurrentPlayerCount = 0;
	bReplicates = true;
}

void ATWLobbyGameState::SetCurrentPlayerCount(int32 InCurrentPlayerCount)
{
	CurrentPlayerCount = InCurrentPlayerCount;

	if (HasAuthority())
	{
		OnRep_CurrentPlayerCount();
	}
}

void ATWLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWLobbyGameState, CurrentPlayerCount);
}

void ATWLobbyGameState::OnRep_CurrentPlayerCount()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr);
	if (!LPC)
	{
		return;
	}

	LPC->RefreshLobbyWidget();
}