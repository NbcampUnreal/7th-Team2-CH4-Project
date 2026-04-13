// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "Core/TWPlayerController.h"

#include "Subsystems/TWUnitSubsystem.h"
#include "MassEntityHandle.h"
#include "MassEntityTraitBase.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "StructUtils/SharedStruct.h"
#include "TWUnitFragment.generated.h"

/**
 * 
 */



USTRUCT()
struct FTWUnitFragment : public FMassFragment
{
	GENERATED_BODY()

	FTWUnitFragment() = default;
	

	int32 GetOwner() const { return Owner; }
	void SetOwner(int32 InOwner) { Owner = InOwner; }
	int32 GetIdx()const { return Idx; }
	void SetIdx(int32 InIdx) { Idx = InIdx; }
	FName GetUnitID()const { return UnitID; }
	void SetUnitID(FName InUnitID) { UnitID = InUnitID; }
protected:
	UPROPERTY(Transient)
	int32 Owner = -1;
	UPROPERTY(Transient)
	int32 Idx = -1;
	UPROPERTY(Transient)
	FName UnitID;
};

