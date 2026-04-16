// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobbyGameMode.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Lobby/TWLobbyGameState.h"
#include "Lobby/TWLobbyPlayerController.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"

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
	
	ATWLobbyGameState* GS = GetGameState<ATWLobbyGameState>();
	if (!GS) return;
	
	if (!IsValid(NewPlayer)) return;
	
	if (IsValid(NewPlayer))
	{
		ATWLobbyPlayerState* LPS = NewPlayer->GetPlayerState<ATWLobbyPlayerState>();
		
		if (LPS)
		{
			FString JoinedNickname = UGameplayStatics::ParseOption(OptionsString, TEXT("Name"));
			
			if (JoinedNickname.IsEmpty())
			{
				JoinedNickname = FString::Printf(TEXT("Player_%d"), GetNumPlayers());
			}
			
			LPS->SetMyNickName(JoinedNickname);
			
			if (GS->PlayerArray.Num() == 1)
			{
				LPS->SetIsHost(true);
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

void ATWLobbyGameMode::AssignNewHost()
{
	bUseSeamlessTravel = true;
	GetWorld()->ServerTravel(TEXT("NewMap?listen"));
}
