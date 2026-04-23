// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "MassEntityTraitBase.h"
#include "Mass/Fragments/TWAvoidanceFragments.h"
#include "TWAvoidanceTrait.generated.h"


UCLASS(meta = (DisplayName = "TWAvoidance"))
class UTWObstacleAvoidanceTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	UPROPERTY(EditAnywhere, Category="")
	FTWMovingAvoidanceParameters MovingParameters;
	
	UPROPERTY(EditAnywhere, Category="")
	FTWStandingAvoidanceParameters StandingParameters;
};

