#pragma once

#include "CoreMinimal.h"
#include "TWPlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "TWGameMode.generated.h"

class ATWPlayerState;
class ATWBaseBuilding;
class APlayerController;

UCLASS()
class CH4_PROJECT_API ATWGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	void TryBindBuilding(ATWBaseBuilding* InBuilding);
	void HandlePlayerDefeat(int32 DefeatedPlayerSlot);

protected:
	void BindPlacedBuildingsForPlayer(ATWPlayerState* InPlayerState);
	ATWPlayerState* FindPlayerStateBySlot(int32 InPlayerSlot) const;
};