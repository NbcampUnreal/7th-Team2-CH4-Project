// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleHandler.h"
#include "Mass/Replication/Agent/TransformReplicatedAgent.h"
#include "TransformMassFastArrayItem.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTransformMassFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()
	
	FTransformMassFastArrayItem() = default;  
	FTransformMassFastArrayItem(const FTransformReplicatedAgent& InAgent, const FMassReplicatedAgentHandle InHandle)  
		: FMassFastArrayItemBase(InHandle), Agent(InAgent) {}  
    
	/** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */  
	typedef FTransformReplicatedAgent FReplicatedAgentType;  
    
	UPROPERTY()  
	FTransformReplicatedAgent Agent;  
};
