// Fill out your copyright notice in the Description page of Project Settings.


#include "UnitSubsystem.h"

#include "MassReplicationSubsystem.h"
#include "Mass/Replication/BubbleInfo/TransformMassClientBubbleInfo.h"
#include "Mass/Replication/BubbleInfo/TransformSmoothMassClientBubbleInfo.h"

void UUnitSubsystem::PostInitialize()
{
	Super::PostInitialize();
	const UWorld* World = GetWorld();  
	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(World);
	ReplicationSubsystem->RegisterBubbleInfoClass(ATransformMassClientBubbleInfo::StaticClass());
	ReplicationSubsystem->RegisterBubbleInfoClass(ATransformSmoothMassClientBubbleInfo::StaticClass());
}
