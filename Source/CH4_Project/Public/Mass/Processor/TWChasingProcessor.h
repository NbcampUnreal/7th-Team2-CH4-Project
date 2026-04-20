// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "TWChasingProcessor.generated.h"

UCLASS()
class CH4_PROJECT_API UTWChasingProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UTWChasingProcessor();

protected:
	FMassEntityQuery EntityQuery;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
};
