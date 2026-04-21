// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "MassEntityTraitBase.h"
#include "Mass/Fragments/TWSteeringFragments.h"
#include "TWSteeringTrait.generated.h"

#define UE_API MASSNAVIGATION_API


UCLASS(meta = (DisplayName = "TWSteering"))
class UTWSteeringTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;

	UPROPERTY(Category="Steering", EditAnywhere, meta=(EditInline))
	FTWMovingSteeringParameters MovingSteering;

	UPROPERTY(Category="Steering", EditAnywhere, meta=(EditInline))
	FTWStandingSteeringParameters StandingSteering;
};

#undef UE_API
