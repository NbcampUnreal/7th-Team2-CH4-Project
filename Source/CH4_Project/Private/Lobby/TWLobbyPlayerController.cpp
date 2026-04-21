// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Lobby/TWLobbyGameMode.h"
#include "Lobby/TWLobby_Layout.h"
#include "Blueprint/UserWidget.h"


void ATWLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController())
	{
		CreateLobbyWidget();
	}
}

// --- 준비 상태 설정 요청 RPC ---
bool ATWLobbyPlayerController::Server_SetReady_Validate(bool bNewReady)
{
	return true;
}

void ATWLobbyPlayerController::Server_SetReady_Implementation(bool bNewReady)
{
	ATWLobbyPlayerState* LPS = GetPlayerState<ATWLobbyPlayerState>();
	if (LPS)
	{
		LPS->SetIsReady(bNewReady);
	}
}



// --- 게임 시작 요청 RPC ---
bool ATWLobbyPlayerController::Server_RequestStartGame_Validate()
{
	return true;
}

void ATWLobbyPlayerController::Server_RequestStartGame_Implementation()
{
	ATWLobbyGameMode* LGM = GetWorld()->GetAuthGameMode<ATWLobbyGameMode>();
	if (LGM)
	{
		if (LGM->CheckStartCondition())
		{
			LGM->StartGame();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Can Not Start!!!"));
		}
	}
}


void ATWLobbyPlayerController::ExitLobby()
{
	ClientTravel(TEXT("L_Title"), ETravelType::TRAVEL_Absolute);
}

void ATWLobbyPlayerController::CreateLobbyWidget()
{
	if (LobbyWidgetInstance) return;
	
	if (LobbyWidgetClass)
	{
		LobbyWidgetInstance = CreateWidget<UTWLobby_Layout>(this, LobbyWidgetClass);
		if (LobbyWidgetInstance)
		{
			LobbyWidgetInstance->AddToViewport();
			bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(LobbyWidgetInstance->GetCachedWidget());
			SetInputMode(InputMode);
		}
	}
}
