#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TWPlayerSlotInterface.generated.h"

UINTERFACE(MinimalAPI)
class UTWPlayerSlotInterface : public UInterface
{
	GENERATED_BODY()
};

class CH4_PROJECT_API ITWPlayerSlotInterface
{
	GENERATED_BODY()

public:
	virtual void UpdatePlayerSlot(int32 newPlayerSlot) = 0;
};
