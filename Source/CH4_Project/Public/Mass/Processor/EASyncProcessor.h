// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassTranslator.h"
#include "EASyncProcessor.generated.h"

UCLASS()
class CH4_PROJECT_API UEASyncProcessor : public UMassTranslator
{
	GENERATED_BODY()
public:
	UEASyncProcessor();

protected:
	FMassEntityQuery EntityQuery;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
};
