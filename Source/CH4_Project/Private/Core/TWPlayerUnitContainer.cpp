// Fill out your copyright notice in the Description page of Project Settings.


#include "TWPlayerUnitContainer.h"

#include "MassReplicationFragments.h"
#include "MassReplicationSubsystem.h"
#include "Core/TWGameState.h"
#include "Data/TWUnitTableRowBase.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Subsystems/TWUnitSubsystem.h"

void UTWPlayerUnitContainer::Init(int32 InOwnerSlot)
{
	MassReplicationSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
	OwnerSlot = InOwnerSlot;
}



void UTWPlayerUnitContainer::SetOwnerSlot(int32 InOwnerSlot)
{
	OwnerSlot =	InOwnerSlot;
}

FMassEntityHandle UTWPlayerUnitContainer::GetEntityHandle(int32 Idx) const
{
	return MassReplicationSubsystem->GetEntityInfoMap()[Units[Idx]].Entity;
}


void UTWPlayerUnitContainer::AddUnit(FMassEntityHandle& Unit)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));
	ATWGameState* GameState =  Cast<ATWGameState>(GetWorld()->GetGameState());
	if (false == IsValid(GameState))
	{
		return;
	}
	
	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(this);
	Units.Add(EntityManager->GetFragmentDataPtr<FMassNetworkIDFragment>(Unit)->NetID);

	float UpkeepRatio = GameState->GetUpkeepRatio();
	if (FTWUnitFragment* UnitFragment =EntityManager->GetFragmentDataPtr<FTWUnitFragment>(Unit))
	{
		UnitFragment->SetIdx(Units.Num()-1);
		UnitFragment->SetOwner(OwnerSlot);
		UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
		if (IsValid(UnitSubsystem)){
			FTWUnitTableRowBase* UnitTableRowBase = UnitSubsystem->GetUnitTableRowBase(UnitFragment->GetUnitID());
			if (nullptr != UnitTableRowBase)
			{
				for (TPair<EResourceType, int> Pair : UnitTableRowBase->Cost)
				{
					Upkeep[Pair.Key] += Pair.Value * UpkeepRatio;
				}	
			}
		}
	}
}

void UTWPlayerUnitContainer::RemoveUnit(int32 Idx)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));
	if (Idx == Units.Num()-1)
	{
		Units.RemoveAt(Idx);
		return;
	}
	
	Units.RemoveAtSwap(Idx);
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
	UMassReplicationSubsystem* RepSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
	FMassEntityHandle EntityHandle = RepSubsystem->GetEntityInfoMap()[Units[Idx]].Entity;
	if (EntityManager.IsEntityActive(EntityHandle))
	{
		if (FTWUnitFragment* OwnerFragment =EntityManager.GetFragmentDataPtr<FTWUnitFragment>(EntityHandle))
		{
			OwnerFragment->SetIdx(Idx);
		}
	}
}

