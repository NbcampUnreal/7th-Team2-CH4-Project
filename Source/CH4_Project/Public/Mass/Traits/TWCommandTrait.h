// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassEntityTraitBase.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "StructUtils/SharedStruct.h"
#include "TWCommandTrait.generated.h"

/**
 * 
 */

USTRUCT()
struct FTWMassMovingTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FTWMassSearchingTag : public FMassTag
{
	GENERATED_BODY()
};

USTRUCT()
struct FTWMassHoldTag : public FMassTag
{
	GENERATED_BODY()
};

UCLASS()
class CH4_PROJECT_API UTWCommandTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
