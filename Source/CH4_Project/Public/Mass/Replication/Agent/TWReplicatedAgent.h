// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassReplicationTransformHandlers.h"
#include "MassReplicationTypes.h"
#include "Data/TWUnitStatus.h"
#include "TWReplicatedAgent.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWReplicatedAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()
	
#pragma region Status
	const FTWUnitStatus& GetStatus() const { return Status; }  
 
	void SetStatus(const FTWUnitStatus& InStatus) { Status = InStatus; }  
#pragma endregion
	
#pragma region Transform
	const FReplicatedAgentPositionYawData& GetPositionYawData() const { return PositionYawData; }  
 
	void SetPositionYawData(const FReplicatedAgentPositionYawData& InPositionYawData) { PositionYawData = InPositionYawData; }  
#pragma endregion
	
#pragma region Unit
	int32 GetOwner() const { return Owner; }  
	void SetOwner(const int32 InOwner) { Owner = InOwner; }  

	FName GetUnitID() const { return UnitID; }  
	void SetUnitID(const FName InUnitID) { UnitID = InUnitID; }  
#pragma endregion
	
#pragma region Attack
	float GetLastAttackTime()const{return LastAttackTime;}
	void SetLastAttackTime(const float InLastAttackTime){LastAttackTime = InLastAttackTime;}
#pragma endregion
	
private:  
	UPROPERTY(Transient)  
	FTWUnitStatus Status;
	UPROPERTY(Transient)  
	FReplicatedAgentPositionYawData PositionYawData;
	UPROPERTY(Transient)  
	int32 Owner = -1;
	UPROPERTY(Transient)
	FName UnitID;
	UPROPERTY(Transient)
	float LastAttackTime=0.0f;
};

