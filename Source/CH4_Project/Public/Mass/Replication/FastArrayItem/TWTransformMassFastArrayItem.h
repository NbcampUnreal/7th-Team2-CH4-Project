// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleHandler.h"
#include "Mass/Replication/Agent/TWTransformReplicatedAgent.h"
#include "TWTransformMassFastArrayItem.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWTransformMassFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()
	
	FTWTransformMassFastArrayItem() = default;  
	FTWTransformMassFastArrayItem(const FTWTransformReplicatedAgent& InAgent, const FMassReplicatedAgentHandle InHandle)  
		: FMassFastArrayItemBase(InHandle), Agent(InAgent) {}  
    
	/** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */  
	typedef FTWTransformReplicatedAgent FReplicatedAgentType;  
    
	UPROPERTY()  
	FTWTransformReplicatedAgent Agent;  
};
