// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/TWCommandTrait.h"

#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "Mass/Fragments/TWTransformOffsetParams.h"


void UTWCommandTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
                                    const UWorld& World) const
{

	BuildContext.AddFragment<FTWCommandFragment>();
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	//TODO MOVE 
	FTWTransformOffsetParams OffsetParamsFragment; 
	const FConstSharedStruct OffsetConstSharedStruct = EntityManager.GetOrCreateConstSharedFragment(OffsetParamsFragment);
	BuildContext.AddConstSharedFragment(OffsetConstSharedStruct);
}

