// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "Mass/Replication/Agent/TWTransformReplicatedAgent.h"
#include "TWTransformOffsetFragment.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWTransformOffsetFragment : public FMassFragment
{
	GENERATED_BODY()
	
	UPROPERTY(Transient)  
	FReplicatedAgentPositionYawData TransformOffset;  
};




