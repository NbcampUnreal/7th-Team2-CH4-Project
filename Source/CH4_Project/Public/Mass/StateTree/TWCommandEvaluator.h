// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "UObject/Object.h"
#include "TWCommandEvaluator.generated.h"

USTRUCT()
struct FTWCommandEvaluatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	ETWMassCommand CurrentCommand = ETWMassCommand::None;
};

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWCommandEvaluator : public FMassStateTreeEvaluatorBase
{
	GENERATED_BODY()
	using FInstanceDataType = FTWCommandEvaluatorInstanceData;
protected:
	TStateTreeExternalDataHandle<FTWCommandTypeFragment> CommandTypeHandle;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    
	virtual const UScriptStruct* GetInstanceDataType() const override { return FTWCommandEvaluatorInstanceData::StaticStruct(); }
};
