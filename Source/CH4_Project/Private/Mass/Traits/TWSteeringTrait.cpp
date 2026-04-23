// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mass/Traits/TWSteeringTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "Mass/Fragments/TWSteeringFragments.h"
#include "Engine/World.h"
#include "MassEntityUtils.h"


void UTWSteeringTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	BuildContext.RequireFragment<FAgentRadiusFragment>();
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassVelocityFragment>();
	BuildContext.RequireFragment<FMassForceFragment>();

	BuildContext.AddFragment<FMassMoveTargetFragment>();
	BuildContext.AddFragment<FTWSteeringFragment>();
	BuildContext.AddFragment<FTWStandingSteeringFragment>();
	BuildContext.AddFragment<FMassGhostLocationFragment>();

	const FConstSharedStruct MovingSteeringFragment = EntityManager.GetOrCreateConstSharedFragment(MovingSteering);
	BuildContext.AddConstSharedFragment(MovingSteeringFragment);

	const FConstSharedStruct StandingSteeringFragment = EntityManager.GetOrCreateConstSharedFragment(StandingSteering);
	BuildContext.AddConstSharedFragment(StandingSteeringFragment);
}
