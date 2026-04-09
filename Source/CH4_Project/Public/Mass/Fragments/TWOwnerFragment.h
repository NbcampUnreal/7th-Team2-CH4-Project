// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityElementTypes.h"
#include "Core/TWPlayerController.h"

#include "MassEntityHandle.h"
#include "MassEntityTraitBase.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "StructUtils/SharedStruct.h"
#include "TWOwnerFragment.generated.h"

/**
 * 
 */



USTRUCT()
struct FTWOwnerFragment : public FMassFragment
{
	GENERATED_BODY()

	FTWOwnerFragment() = default;
	

	ATWPlayerController* GetOwner() const { return Owner.Get(); }
	void SetOwner(ATWPlayerController* InOwner) { Owner = InOwner; }

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<ATWPlayerController> Owner;
};

