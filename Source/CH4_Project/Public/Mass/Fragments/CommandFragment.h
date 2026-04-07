// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassEntityTraitBase.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "StructUtils/SharedStruct.h"
#include "CommandFragment.generated.h"

/**
 * 
 */
UENUM()
enum class ETWCommandType:uint8
{
	None,
	MoveToLocation,
};


USTRUCT()
struct FTWCommandTypeFragment : public FMassFragment
{
	GENERATED_BODY()

	FTWCommandTypeFragment() = default;

	FTWCommandTypeFragment(const ETWCommandType InType)
		: Type(InType)
	{
	}

	ETWCommandType GetType() const { return Type; }
	void SetType(const ETWCommandType InType) { Type = InType; }

protected:
	UPROPERTY(Transient)
	ETWCommandType Type = ETWCommandType::None;
};

USTRUCT()
struct FTWCommandDataFragment : public FMassSharedFragment
{
	GENERATED_BODY()
	FTWCommandDataFragment() = default;

	FTWCommandDataFragment(const FVector& InLocation, const FMassEntityHandle& InTarget)
		: Location(InLocation),
		  Target(InTarget)
	{
	}

	FVector GetLocation() const { return Location; }
	void SetLocation(const FVector& InLocation) { Location = InLocation; }

	FMassEntityHandle GetTarget() const { return Target; }
	void SetTarget(const FMassEntityHandle& InTarget) { Target = InTarget; }

protected:
	UPROPERTY(Transient)
	FVector Location = {0,0,0};
	UPROPERTY(Transient)
	FMassEntityHandle Target;
};

