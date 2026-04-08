#pragma once

#include "CoreMinimal.h"
#include "TWPlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "TWGameMode.generated.h"

UCLASS()
class CH4_PROJECT_API ATWGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	void BindPlacedBuildingsForPlayer(ATWPlayerState* InPlayerState);
};