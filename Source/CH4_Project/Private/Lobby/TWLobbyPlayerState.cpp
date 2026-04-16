// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyPlayerState.h"

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobby_Layout.h"
#include "Net/UnrealNetwork.h"

ATWLobbyPlayerState::ATWLobbyPlayerState()
{
	bIsReady = false;
	bIsHost = false;
	MyNickName = TEXT("Default_Player");
	
	bReplicates = true;
}

void ATWLobbyPlayerState::SetMyNickName(const FString& InNickName)
{
	MyNickName = InNickName;
	
	UE_LOG(LogTemp, Log, TEXT("NickName Set : %s"), *MyNickName);
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
	DOREPLIFETIME(ATWLobbyPlayerState, MyNickName);
	DOREPLIFETIME(ATWLobbyPlayerState, bIsHost);
}

void ATWLobbyPlayerState::OnRep_IsReady()
{
	UE_LOG(LogTemp, Log, TEXT("Client: %s's Ready State is now %s"), *MyNickName, bIsReady ? TEXT("True") : TEXT("False"));
	
	ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetWorld()->GetFirstPlayerController());
	
	if (PC && PC->LobbyWidgetInstance)
	{
		PC->LobbyWidgetInstance->UpdateUserList(MyNickName);
	}
}

void ATWLobbyPlayerState::OnRep_MyNickName()
{
	UE_LOG(LogTemp, Log, TEXT("Client: Nickname Updated: %s"), *MyNickName);
}

void ATWLobbyPlayerState::OnRep_IsHost()
{
	if (bIsHost)
	{
		UE_LOG(LogTemp, Log, TEXT("Client: I am the Host now!"));
		
		
	}
}
