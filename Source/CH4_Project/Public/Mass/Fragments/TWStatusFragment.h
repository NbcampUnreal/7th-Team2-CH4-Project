// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassEntityTraitBase.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "StructUtils/SharedStruct.h"
#include "TWStatusFragment.generated.h"

/**
 * 
 */


USTRUCT(BlueprintType)
struct FTWStatusFragment : public FMassFragment
{
	GENERATED_BODY()

	FTWStatusFragment() = default;

	float GetHealth() const { return Health; }
	float GetDamage() const { return Damage; }
	float GetRange() const { return Range; }
	void SetHealth(float InHealth){Health = InHealth;}
	void SetDamage(float InDamage){Damage = InDamage;}
	void SetRange(float InRange){Range = InRange;}

protected:
	UPROPERTY(Transient)
	float Health=100;
	UPROPERTY(Transient)
	float Damage=100;
	UPROPERTY(Transient)
	float Range=100;
};
