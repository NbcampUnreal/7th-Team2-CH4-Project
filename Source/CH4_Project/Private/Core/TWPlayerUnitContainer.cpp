#include "TWPlayerUnitContainer.h"

#include "MassReplicationFragments.h"
#include "MassReplicationSubsystem.h"
#include "Core/TWGameState.h"
#include "Core/TWPlayerState.h"
#include "Data/TWUnitTableRowBase.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Subsystems/TWUnitSubsystem.h"

void UTWPlayerUnitContainer::Init(int32 InOwnerSlot)
{
	OwnerSlot = InOwnerSlot;
	CurrentPopulation = 0;
	UEnum* EnumPtr = StaticEnum<EResourceType>();
	if (EnumPtr)
	{
		for (int32 i = 0; i < (int32)EResourceType::Count; ++i)
		{
			EResourceType EnumValue = static_cast<EResourceType>(i);
			Upkeep.Add(EnumValue);
		}
	}

}


void UTWPlayerUnitContainer::SetOwnerSlot(int32 InOwnerSlot)
{
	OwnerSlot = InOwnerSlot;
}

FMassEntityHandle UTWPlayerUnitContainer::GetEntityHandle(int32 Idx) const
{
	return Units[Idx];
	// UMassReplicationSubsystem* MassReplicationSubsystem = GetWorld()->GetSubsystem<UMassReplicationSubsystem>();
	// return MassReplicationSubsystem->GetEntityInfoMap()[Units[Idx]].Entity;
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

void UTWPlayerUnitContainer::ApplyStatus(FName UnitID, const FTWUnitStatus& UnitStatus)
{
	FMassEntityManager* MassEntityManager = UE::Mass::Utils::GetEntityManager(GetWorld());
	if (!MassEntityManager)
	{
		return;
	}
	for (const FMassEntityHandle& Unit : Units)
	{
		FTWUnitFragment* UnitFragment = MassEntityManager->GetFragmentDataPtr<FTWUnitFragment>(Unit);
		if (UnitFragment)
		{
			if (UnitFragment->GetUnitID() == UnitID)
			{
				FTWStatusFragment* StatusFragment = MassEntityManager->GetFragmentDataPtr<FTWStatusFragment>(Unit);
				if (StatusFragment)
				{
					StatusFragment->SetStatus(UnitStatus);
					for (int32 i = 0; i < static_cast<int32>(ETWStatusType::Count); i++)
					{
						if ((ETWStatusType)i != ETWStatusType::Health)
						{
							StatusFragment->GetMutableStatus().SetStatus(
								static_cast<ETWStatusType>(i),UnitStatus.GetStatus(static_cast<ETWStatusType>(i)));
						}
					}
					//TODO Applay Move Speed
				}
			}
		}
	}
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
}

void UTWPlayerUnitContainer::IncreasePopulation(const int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentPopulation += Amount;
}

void UTWPlayerUnitContainer::DecreasePopulation(const int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	CurrentPopulation -= Amount;
	CurrentPopulation = FMath::Max(0, CurrentPopulation);
}


void UTWPlayerUnitContainer::AddUnit(FMassEntityHandle& Unit)
{
	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(GetOuter());
	// Units.Add(EntityManager->GetFragmentDataPtr<FMassNetworkIDFragment>(Unit)->NetID);
	Units.Add(Unit);
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
				SyncCachedValuesToPlayerState();
			}
		}
	}
}

void UTWPlayerUnitContainer::RemoveUnit(int32 Idx)
{
	checkf(GetWorld()->GetAuthGameMode(), TEXT("Server Logic Called!"));
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
	FMassEntityHandle EntityHandle = GetEntityHandle(Idx);
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
				SyncCachedValuesToPlayerState();
			}
		}
	}
	if (Idx == Units.Num() - 1)
	{
		Units.RemoveAt(Idx);
		return;
	}

	Units.RemoveAtSwap(Idx);
	// FMassEntityHandle SwapedEntityHandle = RepSubsystem->GetEntityInfoMap()[Units[Idx]].Entity;
	FMassEntityHandle SwapedEntityHandle = GetEntityHandle(Idx);
	if (EntityManager.IsEntityActive(SwapedEntityHandle))
	{
		if (FTWUnitFragment* UnitFragment = EntityManager.GetFragmentDataPtr<FTWUnitFragment>(SwapedEntityHandle))
		{
			UnitFragment->SetIdx(Idx);
		}
	}
}