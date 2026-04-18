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

// 수정x
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

// 수정 필요
void ATWLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	UE_LOG(LogTemp, Error, TEXT("!!! C++ PostLogin EXECUTED !!! Target: %s"), *NewPlayer->GetName());
	
	ATWLobbyGameState* LGS = GetGameState<ATWLobbyGameState>();
	if (!LGS || !NewPlayer) return;
	
	if (!IsValid(NewPlayer)) return;
	
	if (IsValid(NewPlayer))
	{
		ATWLobbyPlayerState* LPS = NewPlayer->GetPlayerState<ATWLobbyPlayerState>();
		
		if (LPS)
		{
			if (GetNumPlayers() == 1)
			{
				UE_LOG(LogTemp, Error, TEXT("Player Numbers : %d"), GetNumPlayers());
				LPS->SetIsHost(true);
				UE_LOG(LogTemp, Error, TEXT("!!! SetIsHost :  %d!!!"), LPS->IsHost());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Player Numbers : %d"), GetNumPlayers());
				LPS->SetIsHost(false);
				UE_LOG(LogTemp, Error, TEXT("!!! SetIsHost :  %d!!!"), LPS->IsHost());
			}
		}
	}
	CheckStartCondition();
}

// 수정x
void ATWLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	AssignNewHost();
	CheckStartCondition();
}

// 수정x
bool ATWLobbyGameMode::CheckStartCondition()
{
	bAllReady = true;
	
	ATWLobbyGameState* LGS = Cast<ATWLobbyGameState>(GetWorld()->GetGameState());
	if (!LGS)
	{
		return false;
	}
	
	// 조건 1 : 최소 인원수 (2명)
	if (LGS->PlayerArray.Num() < MinPlayersToStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("To Start Play need at least 2 player"));
		return false;
	}
	
	// 조건 2 : 방장 제외 전원 준비 완료
	for (APlayerState* PS : LGS->PlayerArray)
	{
		ATWLobbyPlayerState* LPS = Cast<ATWLobbyPlayerState>(PS);
		if (LPS)
		{
			if (!LPS->IsHost() && !LPS->IsReady())
			{
				bAllReady = false;
				return false;
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("CheckStartCondition: %s"), bAllReady ? TEXT("true") : TEXT("false"));
	return bAllReady;
}

// 수정x
void ATWLobbyGameMode::StartGame()
{
	bUseSeamlessTravel = true;
	GetWorld()->ServerTravel(TEXT("L_Main?listen"));
}

// 수정 필요
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
