// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mass/Traits/TWAvoidanceTrait.h"
#include "Mass/Fragments/TWAvoidanceFragments.h"
#include "MassEntityTemplateRegistry.h"
#include "MassMovementFragments.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "Engine/World.h"
#include "MassEntityUtils.h"


void UTWObstacleAvoidanceTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.RequireFragment<FAgentRadiusFragment>();
	BuildContext.AddFragment<FTWNavigationEdgesFragment>();
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassVelocityFragment>();
	BuildContext.RequireFragment<FMassForceFragment>();
	BuildContext.RequireFragment<FMassMoveTargetFragment>();

	const FTWMovingAvoidanceParameters MovingValidated = MovingParameters.GetValidated();
	const FConstSharedStruct MovingFragment = EntityManager.GetOrCreateConstSharedFragment(MovingValidated);
	BuildContext.AddConstSharedFragment(MovingFragment);

	const FTWStandingAvoidanceParameters StandingValidated = StandingParameters.GetValidated();
	const FConstSharedStruct StandingFragment = EntityManager.GetOrCreateConstSharedFragment(StandingValidated);
	BuildContext.AddConstSharedFragment(StandingFragment);
}
