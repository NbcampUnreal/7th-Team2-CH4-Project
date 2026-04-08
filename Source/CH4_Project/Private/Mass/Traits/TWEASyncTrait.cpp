// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/TWEASyncTrait.h"

#include "MassEntityTemplateRegistry.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "Mass/Fragments/TWTransformOffsetParams.h"


void UTWEASyncTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
                                   const UWorld& World) const
{

	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassActorFragment>();
	BuildContext.AddTag<FTWEASyncTag>();
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FTWTransformOffsetParams OffsetParamsFragment; 
	const FConstSharedStruct OffsetConstSharedStruct = EntityManager.GetOrCreateConstSharedFragment(OffsetParamsFragment);
	BuildContext.AddConstSharedFragment(OffsetConstSharedStruct);
}
