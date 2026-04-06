// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "Mass/Fragments/CommandFragment.h"
#include "UObject/Object.h"
#include "CommandEvaluator.generated.h"

USTRUCT()
struct FCommandEvaluatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Output")
	ECommandType CurrentCommand = ECommandType::None;
};

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FCommandEvaluator : public FMassStateTreeEvaluatorBase
{
	GENERATED_BODY()
	using FInstanceDataType = FCommandEvaluatorInstanceData;
protected:
	TStateTreeExternalDataHandle<FCommandTypeFragment> CommandTypeHandle;

	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
    
	virtual const UScriptStruct* GetInstanceDataType() const override { return FCommandEvaluatorInstanceData::StaticStruct(); }
};
