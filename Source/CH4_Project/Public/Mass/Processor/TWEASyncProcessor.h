// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassTranslator.h"
#include "TWEASyncProcessor.generated.h"

UCLASS()
class CH4_PROJECT_API UTWEASyncProcessor : public UMassTranslator
{
	GENERATED_BODY()
public:
	UTWEASyncProcessor();

protected:
	FMassEntityQuery EntityQuery;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
};
