#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TWInputKeyHandler.generated.h"

UINTERFACE(MinimalAPI)
class UTWInputKeyHandler : public UInterface
{
	GENERATED_BODY()
};

class CH4_PROJECT_API ITWInputKeyHandler
{
	GENERATED_BODY()

public:
	virtual bool HandleInputKey(const FKey& InKey, APlayerController* InstigatorController) = 0;
};