// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassReplicationTypes.h"
#include "TWUnitReplicatedAgent.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWUnitReplicatedAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()
	
	int32 GetOwner() const { return Owner; }  
	void SetOwner(const int32 InOwner) { Owner = InOwner; }  

	FName GetUnitID() const { return UnitID; }  
	void SetUnitID(const FName InUnitID) { UnitID = InUnitID; }  
  
	
private:  
	UPROPERTY(Transient)  
	int32 Owner = -1;
	UPROPERTY(Transient)
	FName UnitID;
};

