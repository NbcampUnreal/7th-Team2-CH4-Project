// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Traits/CommandTrait.h"

#include "MassEntityTemplateRegistry.h"
#include "Mass/Fragments/CommandFragment.h"
#include "Mass/Fragments/TransformOffsetParams.h"


void UCommandTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext,
                                  const UWorld& World) const
{

	BuildContext.AddFragment<FCommandTypeFragment>();
	BuildContext.RequireFragment<FCommandDataFragment>();
	
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	
	FCommandDataFragment CommandDataFragment;
	FSharedStruct SharadFragment = EntityManager.GetOrCreateSharedFragment(CommandDataFragment);
	BuildContext.AddSharedFragment(SharadFragment);
	//TODO MOVE 
	FTransformOffsetParams OffsetParamsFragment; 
	const FConstSharedStruct OffsetConstSharedStruct = EntityManager.GetOrCreateConstSharedFragment(OffsetParamsFragment);
	BuildContext.AddConstSharedFragment(OffsetConstSharedStruct);
}

