#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "TWAnimPlayFragment.generated.h"

UENUM()
enum class ETWAnimPlayType : uint8
{
	None,
	Attack,
	Move,
	Idle,
	Dead,
};

USTRUCT()
struct FTWAnimPlayFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FTWAnimPlayFragment() : CurrentAnimType(ETWAnimPlayType::Idle){}
	
	void SetAnimType(const ETWAnimPlayType NewType)
	{
		CurrentAnimType = NewType;
	}
	
	ETWAnimPlayType GetAnimType() const
	{
		return CurrentAnimType;
	}
	
protected:
	UPROPERTY()
	ETWAnimPlayType CurrentAnimType = ETWAnimPlayType::Idle;
};
