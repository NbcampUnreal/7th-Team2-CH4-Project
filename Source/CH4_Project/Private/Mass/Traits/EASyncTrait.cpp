// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/EASyncTrait.h"

#include "MassEntityTemplateRegistry.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "Mass/Fragments/TransformOffsetParams.h"


void UEASyncTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
                                 const UWorld& World) const
{

	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassActorFragment>();
	BuildContext.AddTag<FEASyncTag>();
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FTransformOffsetParams OffsetParamsFragment; 
	const FConstSharedStruct OffsetConstSharedStruct = EntityManager.GetOrCreateConstSharedFragment(OffsetParamsFragment);
	BuildContext.AddConstSharedFragment(OffsetConstSharedStruct);
}
