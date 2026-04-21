// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyPlayerState.h"

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobby_Layout.h"
#include "Core/TWPlayerState.h"
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
	OnRep_IsHost();
}

void ATWLobbyPlayerState::SetLobbyNickname(const FString& InNickname)
{
	if (!HasAuthority())
	{
		return;
	}

	LobbyNickname = InNickname.Left(24).TrimStartAndEnd();

	if (LobbyNickname.IsEmpty())
	{
		LobbyNickname = TEXT("Player");
	}

	SetPlayerName(LobbyNickname);
}

void ATWLobbyPlayerState::SetSelectedHeroUnitId(FName InHeroUnitId)
{
	if (!HasAuthority())
	{
		return;
	}

	SelectedHeroUnitId = InHeroUnitId;
}

void ATWLobbyPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ATWPlayerState* GamePS = Cast<ATWPlayerState>(PlayerState);
	if (!GamePS)
	{
		return;
	}

	GamePS->SetLobbyNickname(LobbyNickname);
	GamePS->SetSelectedHeroUnitId(SelectedHeroUnitId);
}

void ATWLobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWLobbyPlayerState, bIsReady);
	DOREPLIFETIME(ATWLobbyPlayerState, bIsHost);
	DOREPLIFETIME(ATWLobbyPlayerState, LobbyNickname);
	DOREPLIFETIME(ATWLobbyPlayerState, SelectedHeroUnitId);
}

void ATWLobbyPlayerState::OnRep_IsReady()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetWorld()->GetFirstPlayerController());
	
	if (LPC && LPC->LobbyWidgetInstance)
	{
		LPC->LobbyWidgetInstance->UpdateUserImage();
	}
}

void ATWLobbyPlayerState::OnRep_IsHost()
{
	APlayerController* PC = GetPlayerController();
	if (!PC) return;
	
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(PC);
	bool Host = IsHost();
	if (LPC && LPC->LobbyWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("Current Host : %s"), IsHost() ? TEXT("True") : TEXT("False"));
		LPC->LobbyWidgetInstance->ShowPlayButton(Host);
		LPC->LobbyWidgetInstance->UpdateUserImage();
	}
}

void ATWLobbyPlayerState::OnRep_LobbyNickname()
{
	if (ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (PC->LobbyWidgetInstance)
		{
			PC->LobbyWidgetInstance->UpdateUserList();
		}
	}
}

void ATWLobbyPlayerState::OnRep_SelectedHeroUnitId()
{
	if (ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (PC->LobbyWidgetInstance)
		{
			PC->LobbyWidgetInstance->UpdateUserList();
		}
	}
}


