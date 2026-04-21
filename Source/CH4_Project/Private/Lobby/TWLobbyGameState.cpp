// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyGameState.h"

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobby_Layout.h"
#include "Net/UnrealNetwork.h"  // DOREPLIFETIME 매크로용

ATWLobbyGameState::ATWLobbyGameState()
{
	
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
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(PC);
	if (LPC)
	{
		if (LPC->LobbyWidgetInstance)
		{
			LPC->LobbyWidgetInstance->UpdateUserList();
		}
	}
}