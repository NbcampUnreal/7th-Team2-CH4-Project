// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTraitBase.h"
#include "MassEntityTypes.h"
#include "EASyncTrait.generated.h"

USTRUCT()
struct FEASyncTag : public FMassTag
{
	GENERATED_BODY()
	
};
 

UCLASS()
class CH4_PROJECT_API UEASyncTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
