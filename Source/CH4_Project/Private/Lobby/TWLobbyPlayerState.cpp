// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyPlayerState.h"

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobby_Layout.h"
#include "Net/UnrealNetwork.h"

ATWLobbyPlayerState::ATWLobbyPlayerState()
{
	bIsReady = false;
	bIsHost = false;
	
	bReplicates = true;
}

void ATWLobbyPlayerState::SetIsReady(bool bInReady)
{
	bIsReady = bInReady;
}

void ATWLobbyPlayerState::SetIsHost(bool bInHost)
{
	bIsHost = bInHost;
}

void ATWLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWLobbyPlayerState, bIsReady);
	DOREPLIFETIME(ATWLobbyPlayerState, bIsHost);
}

void ATWLobbyPlayerState::OnRep_IsReady()
{
	ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetWorld()->GetFirstPlayerController());
	
	if (PC && PC->LobbyWidgetInstance)
	{
		PC->LobbyWidgetInstance->UpdateUserList();
	}
}

void ATWLobbyPlayerState::OnRep_IsHost()
{
	
}

