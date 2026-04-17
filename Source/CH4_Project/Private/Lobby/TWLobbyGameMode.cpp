// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyGameMode.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Lobby/TWLobbyGameState.h"
#include "Lobby/TWLobbyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/TWLobby_Layout.h"

ATWLobbyGameMode::ATWLobbyGameMode()
{
	PlayerControllerClass = ATWLobbyPlayerController::StaticClass();
	PlayerStateClass = ATWLobbyPlayerState::StaticClass();
	GameStateClass = ATWLobbyGameState::StaticClass();
}

void ATWLobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	
	int32 CurrentPlayerCount = GetNumPlayers();
	
	if (CurrentPlayerCount >= 4)
	{
		ErrorMessage = TEXT("Server is Full");
		UE_LOG(LogTemp, Error, TEXT("Login Failed : Server is Full(%d/4)"), CurrentPlayerCount);
	}
}

void ATWLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	UE_LOG(LogTemp, Error, TEXT("!!! C++ PostLogin EXECUTED !!! Target: %s"), *NewPlayer->GetName());
	
	ATWLobbyGameState* GS = GetGameState<ATWLobbyGameState>();
	if (!GS || !NewPlayer) return;
	
	if (!IsValid(NewPlayer)) return;
	
	if (IsValid(NewPlayer))
	{
		ATWLobbyPlayerState* LPS = NewPlayer->GetPlayerState<ATWLobbyPlayerState>();
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		
		if (LPS)
		{
			if (GetNumPlayers() == 1)
			{
				UE_LOG(LogTemp, Error, TEXT("Player Numbers : %d"), GetNumPlayers());
				UE_LOG(LogTemp, Error, TEXT("!!! SetIsHost(true) !!!"));
				LPS->SetIsHost(true);
			}
		}
		
		if (PC && LPS->IsHost() == false)
		{
			ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(PC);
			if (LPC && LPC->LobbyWidgetInstance)
			{
				LPC->LobbyWidgetInstance->ShowPlayButton(false);
			}
		}
	}
	CheckStartCondition();
}

void ATWLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	AssignNewHost();
	CheckStartCondition();
}

void ATWLobbyGameMode::CheckStartCondition()
{
	ATWLobbyGameState* GS = GetGameState<ATWLobbyGameState>();
	if (!GS) return;
	
	if (GS->PlayerArray.Num() < MinPlayersToStart)
	{
		GS->SetCanStartGame(false);
		return;
	}
	
	bool bAllReady = true;
	
	for (APlayerState* PS : GS->PlayerArray)
	{
		ATWLobbyPlayerState* LPS = Cast<ATWLobbyPlayerState>(PS);
		if (LPS)
		{
			if (!LPS->IsHost() && !LPS->IsReady())
			{
				bAllReady = false;
				break;
			}
		}
	}
	GS->SetCanStartGame(bAllReady);
	UE_LOG(LogTemp, Log, TEXT("CheckStartCondition: %s"), bAllReady ? TEXT("READY TO START") : TEXT("NOT READY"));
}

void ATWLobbyGameMode::StartGame()
{
	bUseSeamlessTravel = true;
	GetWorld()->ServerTravel(TEXT("L_Main?listen"));
}

void ATWLobbyGameMode::AssignNewHost()
{
	ATWLobbyGameState* GS = GetGameState<ATWLobbyGameState>();
	if (!GS) return;
	
	if (GS->PlayerArray.Num() <= 0) return;
	
	bool bHasHost = false;
	
	for (APlayerState* PS : GS->PlayerArray)
	{
		ATWLobbyPlayerState* LPS = Cast<ATWLobbyPlayerState>(PS);
		if (LPS && LPS->IsHost())
		{
			bHasHost = true;
			break;
		}
	}
	if (!bHasHost)
	{
		ATWLobbyPlayerState* NewHost = Cast<ATWLobbyPlayerState>(GS->PlayerArray[0]);
		if (NewHost)
		{
			NewHost->SetIsHost(true);
		}
	}
}
