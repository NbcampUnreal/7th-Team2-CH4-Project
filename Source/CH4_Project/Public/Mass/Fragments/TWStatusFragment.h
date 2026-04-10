// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassEntityTraitBase.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Data/TWUnitStatus.h"
#include "StructUtils/SharedStruct.h"
#include "TWStatusFragment.generated.h"

/**
 * 
 */


USTRUCT(BlueprintType)
struct FTWStatusFragment : public FMassFragment
{
	GENERATED_BODY()

	FTWStatusFragment() = default;
	FORCEINLINE FTWUnitStatus& GetMutableStatus(){return CurrentStatus;}
	//TODO 이동속도 적용해야함
	void SetStatus(const FTWUnitStatus& InStatus){CurrentStatus = InStatus;}

protected:
	FTWUnitStatus CurrentStatus;
};
