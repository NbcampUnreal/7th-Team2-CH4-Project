#include "Lobby/TWLobbyGameMode.h"

#include "Lobby/TWLobbyPlayerState.h"
#include "Lobby/TWLobbyGameState.h"
#include "Lobby/TWLobbyPlayerController.h"

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
		UE_LOG(LogTemp, Warning, TEXT("Login Failed : Server is Full(%d/4)"), CurrentPlayerCount);
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

	ATWLobbyPlayerState* LPS = NewPlayer->GetPlayerState<ATWLobbyPlayerState>();
	if (!LPS)
	{
		CheckStartCondition();
		return;
	}

	const int32 JoinedPlayerIndex = LGS->GetCurrentPlayerCount();

	// 첫 입장 플레이어만 호스트
	bool bShouldBeHost = (JoinedPlayerIndex == 1);
	LPS->SetIsHost(bShouldBeHost);

	if (LPS->GetLobbyNickname().IsEmpty())
	{
		const FString DefaultNickname = FString::Printf(TEXT("Tester%d"), JoinedPlayerIndex);
		LPS->SetLobbyNickname(DefaultNickname);
	}

	if (LPS->GetSelectedHeroUnitId().IsNone())
	{
		FName DefaultHeroId = TEXT("DragonKnight");

		switch (JoinedPlayerIndex)
		{
		case 1:
			DefaultHeroId = TEXT("Markman");
			break;
		case 2:
			DefaultHeroId = TEXT("Astrologian");
			break;
		case 3:
			DefaultHeroId = TEXT("DragonKnight");
			break;
		case 4:
			DefaultHeroId = TEXT("DragonKnight");
			break;
		default:
			break;
		}

		LPS->SetSelectedHeroUnitId(DefaultHeroId);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[LobbyDebug] Auto profile assigned | PlayerIndex=%d | Nickname=%s | HeroId=%s"),
			JoinedPlayerIndex,
			*LPS->GetLobbyNickname(),
			*DefaultHeroId.ToString()
		);
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
	}

	AssignNewHost();
	CheckStartCondition();
}

bool ATWLobbyGameMode::CheckStartCondition()
{
	bAllReady = true;

	ATWLobbyGameState* LGS = GetGameState<ATWLobbyGameState>();
	if (!LGS)
	{
		return false;
	}

	if (LGS->PlayerArray.Num() < MinPlayersToStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("To Start Play need at least 2 player"));
		return false;
	}

	for (APlayerState* PS : LGS->PlayerArray)
	{
		ATWLobbyPlayerState* LPS = Cast<ATWLobbyPlayerState>(PS);
		if (!LPS)
		{
			continue;
		}

		if (!LPS->IsHost() && !LPS->IsReady())
		{
			bAllReady = false;
			return false;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("CheckStartCondition: %s"), bAllReady ? TEXT("true") : TEXT("false"));
	return bAllReady;
}

void ATWLobbyGameMode::StartGame()
{
	if (GameLevelName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] StartGame failed: GameLevelName is None"));
		return;
	}

	bUseSeamlessTravel = bUseSeamlessTravelForGame;

	FString TravelURL = GameLevelName.ToString();
	if (bTravelAsListenServer)
	{
		TravelURL += TEXT("?listen");
	}

	UE_LOG(
		LogTemp,
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
	if (!GS || GS->PlayerArray.Num() <= 0)
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
		if (ATWLobbyPlayerState* NewHost = Cast<ATWLobbyPlayerState>(GS->PlayerArray[0]))
		{
			NewHost->SetIsHost(true);
		}
	}
}