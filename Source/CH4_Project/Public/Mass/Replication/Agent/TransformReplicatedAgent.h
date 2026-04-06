// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassReplicationTypes.h"
#include "MassReplicationTransformHandlers.h"
#include "TransformReplicatedAgent.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTransformReplicatedAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()
	
	const FReplicatedAgentPositionYawData& GetPositionYawData() const { return PositionYawData; }  
 
	void SetPositionYawData(const FReplicatedAgentPositionYawData& InPositionYawData) { PositionYawData = InPositionYawData; }  
  
private:  
	UPROPERTY(Transient)  
	FReplicatedAgentPositionYawData PositionYawData;
};

