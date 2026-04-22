// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/TWLobbyGameMode.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Lobby/TWLobbyGameState.h"
#include "Lobby/TWLobbyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/TWLobby_Layout.h"
#include "Log/TWLogCategory.h"

ATWLobbyGameMode::ATWLobbyGameMode()
{
	PlayerControllerClass = ATWLobbyPlayerController::StaticClass();
	PlayerStateClass = ATWLobbyPlayerState::StaticClass();
	GameStateClass = ATWLobbyGameState::StaticClass();
}

void ATWLobbyGameMode::PreLogin(
	const FString& Options,
	const FString& Address,
	const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage
)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	const int32 CurrentPlayerCount = GetNumPlayers();

	if (CurrentPlayerCount >= 4)
	{
		ErrorMessage = TEXT("Server is Full");
		UE_LOG(LogTWLobby, Warning, TEXT("Login Failed : Server is Full(%d/4)"), CurrentPlayerCount);
	}
}

void ATWLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ATWLobbyGameState* LGS = GetGameState<ATWLobbyGameState>();
	if (!IsValid(NewPlayer) || !LGS)
	{
		return;
	}

	LGS->SetCurrentPlayerCount(GetNumPlayers());
	UE_LOG(LogTWLobby, Warning, TEXT("PostLogin : PlayerCount Updated to %d"), LGS->GetCurrentPlayerCount());

	ATWLobbyPlayerState* LPS = NewPlayer->GetPlayerState<ATWLobbyPlayerState>();
	if (!LPS)
	{
		CheckStartCondition();
		return;
	}

	const int32 JoinedPlayerIndex = LGS->GetCurrentPlayerCount();
	LPS->SetIsHost(JoinedPlayerIndex == 1);

	if (LPS->GetLobbyNickname().IsEmpty())
	{
		const FString DebugNickname = FString::Printf(TEXT("Tester%d"), JoinedPlayerIndex);
		LPS->SetLobbyNickname(DebugNickname);
	}

	if (LPS->GetSelectedHeroUnitId().IsNone())
	{
		FName DebugHeroId = NAME_None;

		switch (JoinedPlayerIndex)
		{
		case 1:
			DebugHeroId = TEXT("Markman");
			break;
		case 2:
			DebugHeroId = TEXT("Astrologian");
			break;
		case 3:
			DebugHeroId = TEXT("DragonKnight");
			break;
		case 4:
			DebugHeroId = TEXT("DragonKnight");
			break;
		default:
			DebugHeroId = TEXT("DragonKnight");
			break;
		}

		LPS->SetSelectedHeroUnitId(DebugHeroId);

		UE_LOG(
			LogTWLobby,
			Log,
			TEXT("[LobbyDebug] Auto profile assigned | PlayerIndex=%d | Nickname=%s | HeroId=%s"),
			JoinedPlayerIndex,
			*LPS->GetLobbyNickname(),
			*DebugHeroId.ToString()
		);
	}

	if (GetWorld())
	{
		switch (GetWorld()->GetNetMode())
		{
		case NM_Standalone:
			UE_LOG(LogTWLobby, Warning, TEXT("NM_Standalone"));
			break;
		case NM_Client:
			UE_LOG(LogTWLobby, Warning, TEXT("NM_Client!!"));
			break;
		case NM_ListenServer:
			UE_LOG(LogTWLobby, Warning, TEXT("NM_ListenServer!!"));
			break;
		case NM_DedicatedServer:
			UE_LOG(LogTWLobby, Warning, TEXT("NM_DedicatedServer!!"));
			break;
		default:
			break;
		}
	}

	CheckStartCondition();
}

void ATWLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ATWLobbyGameState* LGS = GetGameState<ATWLobbyGameState>();
	if (LGS)
	{
		LGS->SetCurrentPlayerCount(GetNumPlayers());
		UE_LOG(LogTWLobby, Warning, TEXT("Logout : PlayerCount Updated to %d"), LGS->GetCurrentPlayerCount());
	}

	AssignNewHost();
	CheckStartCondition();
}

bool ATWLobbyGameMode::CheckStartCondition()
{
	bAllReady = true;

	ATWLobbyGameState* LGS = Cast<ATWLobbyGameState>(GetWorld()->GetGameState());
	if (!LGS)
	{
		return false;
	}

	if (LGS->PlayerArray.Num() < MinPlayersToStart)
	{
		UE_LOG(LogTWLobby, Warning, TEXT("To Start Play need at least 2 player"));
		return false;
	}

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

	UE_LOG(LogTWLobby, Log, TEXT("CheckStartCondition: %s"), bAllReady ? TEXT("true") : TEXT("false"));
	return bAllReady;
}

void ATWLobbyGameMode::StartGame()
{
	if (GameLevelName.IsNone())
	{
		UE_LOG(LogTWLobby, Warning, TEXT("[Lobby] StartGame failed: GameLevelName is None"));
		return;
	}

	bUseSeamlessTravel = bUseSeamlessTravelForGame;

	FString TravelURL = GameLevelName.ToString();
	if (bTravelAsListenServer)
	{
		TravelURL += TEXT("?listen");
	}

	UE_LOG(
		LogTWLobby,
		Warning,
		TEXT("[Lobby] StartGame - TravelURL: %s / Seamless: %s"),
		*TravelURL,
		bUseSeamlessTravel ? TEXT("true") : TEXT("false")
	);

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(TravelURL);
	}
}

void ATWLobbyGameMode::AssignNewHost()
{
	ATWLobbyGameState* GS = GetGameState<ATWLobbyGameState>();
	if (!GS)
	{
		return;
	}

	if (GS->PlayerArray.Num() <= 0)
	{
		return;
	}

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
