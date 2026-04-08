#include "Core/TWPlayerState.h"

#include "Data/TWBuildingTypes.h"
#include "Net/UnrealNetwork.h"

ATWPlayerState::ATWPlayerState()
{
	bReplicates = true;

	PlayerSlot = -1;
	Wood = 0;
	Ore = 0;
	CurrentTroopCount = 0;
	PendingTroopCount = 0;
	MaxTroopCount = 1;
}

void ATWPlayerState::SetPlayerSlot(const int32 InPlayerSlot)
{
	PlayerSlot = InPlayerSlot;
}

void ATWPlayerState::AddResource(const EResourceType ResourceType, const int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Amount <= 0)
	{
		return;
	}

	switch (ResourceType)
	{
	case EResourceType::Wood:
		Wood += Amount;
		break;

	case EResourceType::Ore:
		Ore += Amount;
		break;

	default:
		break;
	}
}

int8 ATWPlayerState::CanAffordCost(const int32 InWoodCost, const int32 InOreCost) const
{
	if (Wood < InWoodCost)
	{
		return 0;
	}

	if (Ore < InOreCost)
	{
		return 0;
	}

	return 1;
}

void ATWPlayerState::SpendCost(const int32 InWoodCost, const int32 InOreCost)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CanAffordCost(InWoodCost, InOreCost) == 0)
	{
		return;
	}

	Wood -= InWoodCost;
	Ore -= InOreCost;
}

int8 ATWPlayerState::CanQueueTroop(const int32 InAmount) const
{
	if (InAmount <= 0)
	{
		return 0;
	}

	if ((CurrentTroopCount + PendingTroopCount + InAmount) <= MaxTroopCount)
	{
		return 1;
	}

	return 0;
}

void ATWPlayerState::AddTroopCount(const int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InAmount <= 0)
	{
		return;
	}

	CurrentTroopCount += InAmount;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerSlot: %d | CurrentTroopCount: %d / %d"),
		PlayerSlot,
		CurrentTroopCount,
		MaxTroopCount
	);
}

void ATWPlayerState::RemoveTroopCount(const int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InAmount <= 0)
	{
		return;
	}

	CurrentTroopCount -= InAmount;
	CurrentTroopCount = FMath::Max(0, CurrentTroopCount);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerSlot: %d | CurrentTroopCount: %d / %d"),
		PlayerSlot,
		CurrentTroopCount,
		MaxTroopCount
	);
}

void ATWPlayerState::AddPendingTroopCount(const int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InAmount <= 0)
	{
		return;
	}

	PendingTroopCount += InAmount;
}

void ATWPlayerState::RemovePendingTroopCount(const int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InAmount <= 0)
	{
		return;
	}

	PendingTroopCount -= InAmount;
	PendingTroopCount = FMath::Max(0, PendingTroopCount);
}

void ATWPlayerState::AddPopulationCap(const int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InAmount <= 0)
	{
		return;
	}

	MaxTroopCount += InAmount;
	MaxTroopCount = FMath::Max(1, MaxTroopCount);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerSlot: %d | PopulationCap(MaxTroopCount): %d"),
		PlayerSlot,
		MaxTroopCount
	);
}

void ATWPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWPlayerState, PlayerSlot);
	DOREPLIFETIME(ATWPlayerState, Wood);
	DOREPLIFETIME(ATWPlayerState, Ore);
	DOREPLIFETIME(ATWPlayerState, CurrentTroopCount);
	DOREPLIFETIME(ATWPlayerState, PendingTroopCount);
	DOREPLIFETIME(ATWPlayerState, MaxTroopCount);
}
