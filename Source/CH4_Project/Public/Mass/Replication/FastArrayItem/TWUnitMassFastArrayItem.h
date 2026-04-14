// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleHandler.h"
#include "Mass/Replication/Agent/TWUnitReplicatedAgent.h"
#include "TWUnitMassFastArrayItem.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWUnitMassFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()
	
	FTWUnitMassFastArrayItem() = default;  
	FTWUnitMassFastArrayItem(const FTWUnitReplicatedAgent& InAgent, const FMassReplicatedAgentHandle InHandle)  
		: FMassFastArrayItemBase(InHandle), Agent(InAgent) {}  
    
	/** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */  
	typedef FTWUnitReplicatedAgent FReplicatedAgentType;  
    
	UPROPERTY()  
	FTWUnitReplicatedAgent Agent;  
};
