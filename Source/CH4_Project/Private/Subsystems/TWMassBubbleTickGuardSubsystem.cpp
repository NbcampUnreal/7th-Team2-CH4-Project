// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/TWMassBubbleTickGuardSubsystem.h"

#include "EngineUtils.h"
#include "MassClientBubbleInfoBase.h"
#include "Engine/World.h"

void UTWMassBubbleTickGuardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(
		this,
		&UTWMassBubbleTickGuardSubsystem::OnWorldBeginTearDown
	);
}

void UTWMassBubbleTickGuardSubsystem::Deinitialize()
{
	if (WorldBeginTearDownHandle.IsValid())
	{
		FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);
		WorldBeginTearDownHandle.Reset();
	}

	Super::Deinitialize();
}

void UTWMassBubbleTickGuardSubsystem::OnWorldBeginTearDown(UWorld* World)
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMassClientBubbleInfoBase> It(World); It; ++It)
	{
		if (AActor* BubbleActor = *It)
		{
			BubbleActor->SetActorTickEnabled(false);
		}
	}
}
