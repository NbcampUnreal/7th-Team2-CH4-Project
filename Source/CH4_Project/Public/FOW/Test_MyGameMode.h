#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Test_MyGameMode.generated.h"

UCLASS()
class CH4_PROJECT_API ATest_MyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
protected:
	int32 PlayerCount = 0;
};
