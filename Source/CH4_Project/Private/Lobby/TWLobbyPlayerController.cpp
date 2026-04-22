// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Lobby/TWLobbyGameMode.h"
#include "Lobby/TWLobby_Layout.h"
#include "Blueprint/UserWidget.h"
#include "Log/TWLogCategory.h"

void ATWLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
		CreateLobbyWidget();
		RefreshLobbyWidget();
	}
}

bool ATWLobbyPlayerController::Server_SetReady_Validate(bool bNewReady)
{
	return true;
}

void ATWLobbyPlayerController::Server_SetReady_Implementation(bool bNewReady)
{
	if (ATWLobbyPlayerState* LPS = GetPlayerState<ATWLobbyPlayerState>())
	{
		LPS->SetIsReady(bNewReady);
	}
}

bool ATWLobbyPlayerController::Server_RequestStartGame_Validate()
{
	return true;
}

void ATWLobbyPlayerController::Server_RequestStartGame_Implementation()
{
	ATWLobbyPlayerState* LPS = GetPlayerState<ATWLobbyPlayerState>();
	if (!LPS || !LPS->IsHost())
	{
		UE_LOG(LogTWLobby, Warning, TEXT("Server_RequestStartGame rejected: not host"));
		return;
	}

	ATWLobbyGameMode* LGM = GetWorld()->GetAuthGameMode<ATWLobbyGameMode>();
	if (!LGM)
	{
		return;
	}

	if (LGM->CheckStartCondition())
	{
		LGM->StartGame();
	}
	else
	{
		UE_LOG(LogTWLobby, Warning, TEXT("Can Not Start!!!"));
	}
}

bool ATWLobbyPlayerController::Server_SetLobbyNickname_Validate(const FString& InNickname)
{
	return InNickname.Len() <= 64;
}

void ATWLobbyPlayerController::Server_SetLobbyNickname_Implementation(const FString& InNickname)
{
	if (ATWLobbyPlayerState* LPS = GetPlayerState<ATWLobbyPlayerState>())
	{
		LPS->SetLobbyNickname(InNickname);
	}
}

bool ATWLobbyPlayerController::Server_SetSelectedHeroUnitId_Validate(FName InHeroUnitId)
{
	return true;
}

void ATWLobbyPlayerController::Server_SetSelectedHeroUnitId_Implementation(FName InHeroUnitId)
{
	if (ATWLobbyPlayerState* LPS = GetPlayerState<ATWLobbyPlayerState>())
	{
		LPS->SetSelectedHeroUnitId(InHeroUnitId);
	}
}

void ATWLobbyPlayerController::ExitLobby()
{
	ClientTravel(TEXT("/Game/CH4_Project/Maps/Main/L_Title"), TRAVEL_Absolute);
}

void ATWLobbyPlayerController::CreateLobbyWidget()
{
	if (LobbyWidgetInstance)
	{
		return;
	}
	
	if (!LobbyWidgetClass)
	{
		return;
	}

	LobbyWidgetInstance = CreateWidget<UTWLobby_Layout>(this, LobbyWidgetClass);
	if (!LobbyWidgetInstance)
	{
		return;
	}

	LobbyWidgetInstance->AddToViewport();
	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
}

void ATWLobbyPlayerController::RefreshLobbyWidget()
{
	if (!IsLocalController() || !LobbyWidgetInstance)
	{
		return;
	}

	LobbyWidgetInstance->UpdateUserList();
	LobbyWidgetInstance->UpdateUserImage();

	if (ATWLobbyPlayerState* LPS = GetPlayerState<ATWLobbyPlayerState>())
	{
		LobbyWidgetInstance->ShowPlayButton(LPS->IsHost());
	}
}