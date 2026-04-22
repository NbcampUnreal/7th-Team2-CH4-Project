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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
	int32 MinPlayersToStart = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lobby")
	bool bAllReady = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	FName GameLevelName = TEXT("L_Main");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	bool bTravelAsListenServer = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Travel")
	bool bUseSeamlessTravelForGame = false;
};