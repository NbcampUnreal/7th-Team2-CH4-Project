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

	// 반드시 전체 패키지 경로로 넣기
	// 예: /Game/CH4_Project/Maps/Main/L_Main
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	FString GameLevelPath = TEXT("/Game/CH4_Project/Maps/Main/L_Main");

	// PIE에서 seamless 허용 안 되어 있으면 자동 fallback
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	bool bFallbackToNonSeamlessInPIE = true;

	// 중복 시작 방지
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby|Travel")
	bool bStartGameRequested = false;

	bool CanUseSeamlessTravelInCurrentWorld() const;
};