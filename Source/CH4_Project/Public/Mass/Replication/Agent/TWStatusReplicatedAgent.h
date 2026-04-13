// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassReplicationTypes.h"
#include "Data/TWUnitStatus.h"
#include "TWStatusReplicatedAgent.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWStatusReplicatedAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()
	
	const FTWUnitStatus& GetStatus() const { return Status; }  
 
	void SetStatus(const FTWUnitStatus& InStatus) { Status = InStatus; }  
  
private:  
	UPROPERTY(Transient)  
	FTWUnitStatus Status;
};

