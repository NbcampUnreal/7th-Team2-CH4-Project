// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSignalSubsystem.h"
#include "MassSmartObjectHandler.h"
#include "MassSmartObjectRequest.h"
#include "MassStateTreeTypes.h"
#include "MassNavigationTypes.h"
#include "MassNavigationFragments.h"
#include "Mass/Fragments/TWCommandFragment.h"

#include "TWStateTreeCommandTask.generated.h"

USTRUCT()
struct CH4_PROJECT_API FTWMassFindRandomSpot
{
	GENERATED_BODY()
 
	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation;
};

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWStateTreeCommandTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FTWMassFindRandomSpot;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FTWMassFindRandomSpot::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

protected:
	TStateTreeExternalDataHandle<FTWCommandDataFragment> EntityCommandDataHandle;
	TStateTreeExternalDataHandle<FTWCommandTypeFragment> EntityCommandTypeHandle;
};
