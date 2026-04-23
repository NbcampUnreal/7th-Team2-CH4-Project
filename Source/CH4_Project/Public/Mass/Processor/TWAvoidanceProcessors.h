// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "TWAvoidanceProcessors.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogAvoidance, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAvoidanceVelocities, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAvoidanceAgents, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAvoidanceObstacles, Warning, All);

class UMassNavigationSubsystem;

/** Experimental: move using cumulative forces to avoid close agents */
UCLASS()
class UTWMovingAvoidanceProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTWMovingAvoidanceProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	TObjectPtr<UWorld> World;
	TObjectPtr<UMassNavigationSubsystem> NavigationSubsystem;
	FMassEntityQuery EntityQuery;
};

/** Avoidance while standing. */
UCLASS()
class UTWStandingAvoidanceProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTWStandingAvoidanceProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
	TObjectPtr<UWorld> World;
	TObjectPtr<UMassNavigationSubsystem> NavigationSubsystem;
	FMassEntityQuery EntityQuery;
};
