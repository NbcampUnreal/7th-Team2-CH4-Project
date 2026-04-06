// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSignalSubsystem.h"
#include "MassSmartObjectHandler.h"
#include "MassSmartObjectRequest.h"
#include "MassStateTreeTypes.h"
#include "MassNavigationTypes.h"
#include "MassNavigationFragments.h"
#include "Mass/Fragments/CommandFragment.h"

#include "TESTStateTreeTask.generated.h"

USTRUCT()
struct CH4_PROJECT_API FMassFindRandomSpot
{
	GENERATED_BODY()
 
	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation;
};

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTESTStateTreeTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FMassFindRandomSpot;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FMassFindRandomSpot::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

protected:
	TStateTreeExternalDataHandle<FCommandDataFragment> EntityCommandDataHandle;
	TStateTreeExternalDataHandle<FCommandTypeFragment> EntityCommandTypeHandle;
};
