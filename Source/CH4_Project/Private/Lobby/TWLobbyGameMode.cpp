#include "Lobby/TWLobbyGameMode.h"

#include "Lobby/TWLobbyPlayerState.h"
#include "Lobby/TWLobbyGameState.h"
#include "Lobby/TWLobbyPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "Log/TWLogCategory.h"

ATWLobbyGameMode::ATWLobbyGameMode()
{
	PlayerControllerClass = ATWLobbyPlayerController::StaticClass();
	PlayerStateClass = ATWLobbyPlayerState::StaticClass();
	GameStateClass = ATWLobbyGameState::StaticClass();

	// 기본값은 true로 두되, PIE에서 필요 시 StartGame에서 fallback 처리
	bUseSeamlessTravel = true;
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
	const bool bShouldBeHost = (JoinedPlayerIndex == 1);
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
		case 4:
		default:
			DefaultHeroId = TEXT("DragonKnight");
			break;
		}

		LPS->SetSelectedHeroUnitId(DefaultHeroId);
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
		return false;
	}

	for (APlayerState* PS : LGS->PlayerArray)
	{
		const ATWLobbyPlayerState* LPS = Cast<ATWLobbyPlayerState>(PS);
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

	return bAllReady;
}

bool ATWLobbyGameMode::CanUseSeamlessTravelInCurrentWorld() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (World->WorldType != EWorldType::PIE)
	{
		return true;
	}

	IConsoleVariable* AllowPIECVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("net.AllowPIESeamlessTravel"));

	if (!AllowPIECVar)
	{
		return false;
	}

	return AllowPIECVar->GetInt() != 0;
}

void ATWLobbyGameMode::StartGame()
{
	// 중복 시작 방지
	if (bStartGameRequested)
	{
		UE_LOG(LogTWLobby, Warning, TEXT("StartGame skipped: already requested"));
		return;
	}

	if (GameLevelPath.IsEmpty())
	{
		UE_LOG(LogTWLobby, Error, TEXT("StartGame failed: GameLevelPath is empty"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTWLobby, Error, TEXT("StartGame failed: World is null"));
		return;
	}

	// 서버에서만 실행
	if (GetNetMode() == NM_Client)
	{
		UE_LOG(LogTWLobby, Warning, TEXT("StartGame skipped: called on client"));
		return;
	}

	// 시작 조건 재검증
	if (!CheckStartCondition())
	{
		UE_LOG(LogTWLobby, Warning, TEXT("StartGame failed: CheckStartCondition() == false"));
		return;
	}

	bool bShouldUseSeamless = CanUseSeamlessTravelInCurrentWorld();
	if (!bShouldUseSeamless && bFallbackToNonSeamlessInPIE)
	{
		UE_LOG(
			LogTWLobby,
			Warning,
			TEXT("PIE seamless travel is not enabled. Falling back to non-seamless ServerTravel. Set net.AllowPIESeamlessTravel=1 to test seamless in PIE.")
		);
	}

	bUseSeamlessTravel = bShouldUseSeamless;
	bStartGameRequested = true;

	UE_LOG(
		LogTWLobby,
		Warning,
		TEXT("StartGame ServerTravel: Path=%s, Seamless=%s, WorldType=%d"),
		*GameLevelPath,
		bUseSeamlessTravel ? TEXT("true") : TEXT("false"),
		static_cast<int32>(World->WorldType)
	);

	const bool bTravelStarted = World->ServerTravel(GameLevelPath);

	if (!bTravelStarted)
	{
		UE_LOG(LogTWLobby, Error, TEXT("StartGame failed: ServerTravel returned false"));
		bStartGameRequested = false;
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