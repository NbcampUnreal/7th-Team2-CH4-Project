// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/TWLobbyPlayerState.h"

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobby_Layout.h"
#include "Core/TWPlayerState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

ATWLobbyPlayerState::ATWLobbyPlayerState()
{
	bIsReady = false;
	bIsHost = false;
	bReplicates = true;
}

void ATWLobbyPlayerState::SetIsReady(bool bInReady)
{
	if (!HasAuthority())
	{
		return;
	}

	bIsReady = bInReady;
	OnRep_IsReady();
}

void ATWLobbyPlayerState::SetIsHost(bool bInHost)
{
	if (!HasAuthority())
	{
		return;
	}

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
	OnRep_LobbyNickname();
}

void ATWLobbyPlayerState::SetSelectedHeroUnitId(FName InHeroUnitId)
{
	if (!HasAuthority())
	{
		return;
	}

	SelectedHeroUnitId = InHeroUnitId;
	OnRep_SelectedHeroUnitId();
}

void ATWLobbyPlayerState::PostNetInit()
{
	Super::PostNetInit();
	RefreshLobbyWidget();
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
	RefreshLobbyWidget();
}

void ATWLobbyPlayerState::OnRep_IsHost()
{
	RefreshLobbyWidget();
}

void ATWLobbyPlayerState::OnRep_LobbyNickname()
{
	RefreshLobbyWidget();
}

void ATWLobbyPlayerState::OnRep_SelectedHeroUnitId()
{
	RefreshLobbyWidget();
}

void ATWLobbyPlayerState::RefreshLobbyWidget() const
{
	if (!GetWorld())
	{
		return;
	}

	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!LPC || !LPC->LobbyWidgetInstance)
	{
		return;
	}

	LPC->LobbyWidgetInstance->UpdateUserList();
	LPC->LobbyWidgetInstance->UpdateUserImage();

	if (const ATWLobbyPlayerState* LocalPS = LPC->GetPlayerState<ATWLobbyPlayerState>())
	{
		LPC->LobbyWidgetInstance->ShowPlayButton(LocalPS->IsHost());
	}
}