#include "TWPlayerUnitContainer.h"

#include "MassReplicationFragments.h"
#include "MassReplicationSubsystem.h"
#include "Core/TWGameState.h"
#include "Core/TWPlayerState.h"
#include "Data/TWUnitTableRowBase.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Subsystems/TWUnitSubsystem.h"

void UTWPlayerUnitContainer::Init(int32 InOwnerSlot)
{
	OwnerSlot = InOwnerSlot;
	CurrentPopulation = 0;
}


void UTWPlayerUnitContainer::SetOwnerSlot(int32 InOwnerSlot)
{
	OwnerSlot = InOwnerSlot;
}

FMassEntityHandle UTWPlayerUnitContainer::GetEntityHandle(int32 Idx) const
{
	UMassReplicationSubsystem* MassReplicationSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
	return MassReplicationSubsystem->GetEntityInfoMap()[Units[Idx]].Entity;
}

void UTWPlayerUnitContainer::SyncCachedValuesToPlayerState()
{
	ATWGameState* GameState = Cast<ATWGameState>(GetWorld()->GetGameState());
	if (!IsValid(GameState))
	{
		return;
	}

	ATWPlayerState* PlayerState = GameState->GetPlayerState(OwnerSlot);
	if (!IsValid(PlayerState))
	{
		return;
	}

	PlayerState->SetCurrentPopulationFromContainer(CurrentPopulation);
	PlayerState->SetTotalTroopUpkeepCost(Upkeep);
}

void UTWPlayerUnitContainer::IncreaseUpkeep(TMap<EResourceType, int32> Amount)
{
	ATWGameState* GameState = Cast<ATWGameState>(GetWorld()->GetGameState());
	if (false == IsValid(GameState))
	{
		return;
	}
	float UpkeepRatio = GameState->GetUpkeepRatio();
	
	for (TPair<EResourceType, int> Pair : Amount)
	{
		Upkeep[Pair.Key] += Pair.Value * UpkeepRatio;
	}
	SyncCachedValuesToPlayerState();
}

void UTWPlayerUnitContainer::DecreaseUpkeep(TMap<EResourceType, int32> Amount)
{
	ATWGameState* GameState = Cast<ATWGameState>(GetWorld()->GetGameState());
	if (false == IsValid(GameState))
	{
		return;
	}
	float UpkeepRatio = GameState->GetUpkeepRatio();
	for (TPair<EResourceType, int> Pair : Amount)
	{
		Upkeep[Pair.Key] -= Pair.Value * UpkeepRatio;
	}
	SyncCachedValuesToPlayerState();
}

void UTWPlayerUnitContainer::IncreasePopulation(const int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentPopulation += Amount;
	SyncCachedValuesToPlayerState();
}

void UTWPlayerUnitContainer::DecreasePopulation(const int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentPopulation -= Amount;
	CurrentPopulation = FMath::Max(0, CurrentPopulation);
	SyncCachedValuesToPlayerState();
}


void UTWPlayerUnitContainer::AddUnit(FMassEntityHandle& Unit)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));


	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(this);
	Units.Add(EntityManager->GetFragmentDataPtr<FMassNetworkIDFragment>(Unit)->NetID);

	if (FTWUnitFragment* UnitFragment = EntityManager->GetFragmentDataPtr<FTWUnitFragment>(Unit))
	{
		UnitFragment->SetIdx(Units.Num() - 1);
		UnitFragment->SetOwner(OwnerSlot);
		UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
		if (IsValid(UnitSubsystem))
		{
			FTWUnitTableRowBase* UnitTableRowBase = UnitSubsystem->GetUnitTableRowBase(UnitFragment->GetUnitID());
			if (nullptr != UnitTableRowBase)
			{
				IncreasePopulation(UnitTableRowBase->Population);
				IncreaseUpkeep(UnitTableRowBase->Cost);
			}
		}
	}
}

void UTWPlayerUnitContainer::RemoveUnit(int32 Idx)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
	UMassReplicationSubsystem* RepSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
	FMassEntityHandle EntityHandle = RepSubsystem->GetEntityInfoMap()[Units[Idx]].Entity;
	if (FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(EntityHandle))
	{
		UTWUnitSubsystem* UnitSubsystem = GetWorld()->GetSubsystem<UTWUnitSubsystem>();
		if (IsValid(UnitSubsystem))
		{
			FTWUnitTableRowBase* UnitTableRowBase = UnitSubsystem->GetUnitTableRowBase(UnitFragment->GetUnitID());
			if (nullptr != UnitTableRowBase)
			{
				DecreasePopulation(UnitTableRowBase->Population);
				DecreaseUpkeep(UnitTableRowBase->Cost);
			}
		}
	}
	if (Idx == Units.Num() - 1)
	{
		Units.RemoveAt(Idx);
		return;
	}

	Units.RemoveAtSwap(Idx);
	FMassEntityHandle SwapedEntityHandle = RepSubsystem->GetEntityInfoMap()[Units[Idx]].Entity;
	if (EntityManager.IsEntityActive(SwapedEntityHandle))
	{
		if (FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(SwapedEntityHandle))
		{
			UnitFragment->SetIdx(Idx);
		}
	}
}
