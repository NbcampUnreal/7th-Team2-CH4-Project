// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Processor/TWNavMeshPathFollowProcessor.h"


UTWNavMeshPathFollowProcessor::UTWNavMeshPathFollowProcessor()
{
}

void UTWNavMeshPathFollowProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::ConfigureQueries(EntityManager);
}

void UTWNavMeshPathFollowProcessor::SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context,
	FMassSignalNameLookup& EntitySignals)
{
	Super::SignalEntities(EntityManager, Context, EntitySignals);
}

void UTWNavMeshPathFollowProcessor::InitializeInternal(UObject& Owner,
	const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);
}
