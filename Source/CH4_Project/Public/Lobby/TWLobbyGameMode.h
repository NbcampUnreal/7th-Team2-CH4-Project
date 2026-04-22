#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TWLobbyGameMode.generated.h"

UCLASS()
class CH4_PROJECT_API ATWLobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATWLobbyGameMode();

	virtual void PreLogin(
		const FString& Options,
		const FString& Address,
		const FUniqueNetIdRepl& UniqueId,
		FString& ErrorMessage
	) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable)
	bool CheckStartCondition();

	UFUNCTION(BlueprintCallable)
	void StartGame();

	void AssignNewHost();

protected:
	// 최소 시작 인원
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
	int32 MinPlayersToStart = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby")
	bool bAllReady = false;

	// 블루프린트에서 지정할 게임 시작 레벨명
	// 예: L_TestUI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	FName GameLevelName = TEXT("L_Main");

	// listen 옵션 포함 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	bool bTravelAsListenServer = true;

	// 필요하면 BP에서 seamless on/off도 조절 가능하게
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	bool bUseSeamlessTravelForGame = false;
};