// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleHandler.h"
#include "Mass/Replication/Agent/TWReplicatedAgent.h"
#include "TWMassFastArrayItem.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWMassFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()
	
	FTWMassFastArrayItem() = default;  
	FTWMassFastArrayItem(const FTWReplicatedAgent& InAgent, const FMassReplicatedAgentHandle InHandle)  
		: FMassFastArrayItemBase(InHandle), Agent(InAgent) {}  
    
	/** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */  
	typedef FTWReplicatedAgent FReplicatedAgentType;  
    
	UPROPERTY()  
	FTWReplicatedAgent Agent;  
};
