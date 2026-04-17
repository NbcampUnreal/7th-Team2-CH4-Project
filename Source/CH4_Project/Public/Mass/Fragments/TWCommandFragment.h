// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassEntityTraitBase.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "StructUtils/SharedStruct.h"
#include "TWCommandFragment.generated.h"

/**
 * 
 */
UENUM()
enum class ETWMassCommand:uint8
{
	None,
	MoveToLocation,
	MoveToUnit,
	AttackToLocation,
	AttackToUnit,
	Hold
};
const FName CommandSignal = FName(TEXT("CommandSignal"));
const FName PathInitSignal = FName(TEXT("PathInitSignal"));
const FName MoveCompleteSignal = FName(TEXT("MoveCompleteSignal"));
const FName AttackCompleteSignal = FName(TEXT("AttackCompleteSignal"));


USTRUCT()
struct CH4_PROJECT_API FTWCommandFragment : public FMassFragment
{
	GENERATED_BODY()

	FTWCommandFragment() = default;

	FTWCommandFragment(const ETWMassCommand InType, const FVector& InLocation, const FMassEntityHandle& InTarget)
	: Type(InType),
	Location(InLocation),
	  Target(InTarget)
	{
	}

	ETWMassCommand GetType() const { return Type; }
	void SetType(const ETWMassCommand InType) { Type = InType; }
	FVector GetLocation() const { return Location; }
	void SetLocation(const FVector& InLocation) { Location = InLocation; }
	FMassEntityHandle GetTarget() const { return Target; }
	void SetTarget(const FMassEntityHandle& InTarget) { Target = InTarget; }
	
protected:
	UPROPERTY(Transient)
	ETWMassCommand Type = ETWMassCommand::None;
	UPROPERTY(Transient)
	FVector Location = {0,0,0};
	UPROPERTY(Transient)
	FMassEntityHandle Target;
};

