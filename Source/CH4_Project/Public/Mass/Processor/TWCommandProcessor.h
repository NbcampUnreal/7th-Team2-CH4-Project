// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSignalProcessorBase.h"
#include "TWCommandProcessor.generated.h"

UCLASS()
class CH4_PROJECT_API UTWCommandProcessor : public UMassSignalProcessorBase
{
	GENERATED_BODY()
public:
	UTWCommandProcessor(const FObjectInitializer& ObjectInitializer);

protected:
	FMassEntityQuery EntityQuery;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FMassSignalNameLookup& EntitySignals) override;
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;
};
