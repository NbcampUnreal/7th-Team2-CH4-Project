// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleHandler.h"
#include "Mass/Replication/Agent/TWStatusReplicatedAgent.h"
#include "TWStatusMassFastArrayItem.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWStatusMassFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()
	
	FTWStatusMassFastArrayItem() = default;  
	FTWStatusMassFastArrayItem(const FTWStatusReplicatedAgent& InAgent, const FMassReplicatedAgentHandle InHandle)  
		: FMassFastArrayItemBase(InHandle), Agent(InAgent) {}  
    
	/** This typedef is required to be provided in FMassFastArrayItemBase derived classes (with the associated FReplicatedAgentBase derived class) */  
	typedef FTWStatusReplicatedAgent FReplicatedAgentType;  
    
	UPROPERTY()  
	FTWStatusReplicatedAgent Agent;  
};
