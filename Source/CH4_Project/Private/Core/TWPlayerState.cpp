#include "Core/TWPlayerState.h"
#include "Data/TWBuildingTypes.h"
#include "FOW/TWPlayerSlotInterface.h"
#include "Net/UnrealNetwork.h"
#include "Building/TWTroopSpawnBuilding.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Mass/Fragments/TWOwnerFragment.h"

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

void ATWPlayerState::BeginPlay()
{
	Super::BeginPlay();
	Units.SetNum(MaxTroopCount);
	
}


void ATWPlayerState::SetPlayerSlot(const int32 NewSlot)
{
	if (HasAuthority())
	{
		PlayerSlot = NewSlot;
		OnRep_PlayerSlot();
	}
}

void ATWPlayerState::OnRep_PlayerSlot()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn) {
		return;
	}
	
	if (ITWPlayerSlotInterface* Interface = Cast<ITWPlayerSlotInterface>(MyPawn))
	{
		Interface->UpdatePlayerSlot(PlayerSlot);
	}
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
	RefreshTroopUpkeepTimer();
	
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
	RefreshTroopUpkeepTimer();
	
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

void ATWPlayerState::AddUnit(FMassEntityHandle& Unit)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));
	checkf(Units.Num()<MaxTroopCount, TEXT("MaxTroopCount OverFlow!"));
	Units[CurrentTroopCount] = Unit;
	FMassEntityManager* EntityManager = UE::Mass::Utils::GetEntityManager(this);
	if (FTWOwnerFragment* OwnerFragment =EntityManager->GetFragmentDataPtr<FTWOwnerFragment>(Unit))
	{
		OwnerFragment->SetIdx(CurrentTroopCount);
	}

	++CurrentTroopCount;
}

void ATWPlayerState::RemoveUnit(int32 Idx)
{
	checkf(HasAuthority(), TEXT("Server Logic Called!"));
	
	--CurrentTroopCount;
	Units.Swap(Idx, CurrentTroopCount);
	if (Idx != CurrentTroopCount)
	{
		FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*GetWorld());
		if (EntityManager.IsEntityActive(Units[Idx]))
		{
			if (FTWOwnerFragment* OwnerFragment =EntityManager.GetFragmentDataPtr<FTWOwnerFragment>(Units[Idx]))
			{
				OwnerFragment->SetIdx(Idx);
			}
		}
	}
	Units.RemoveAt(CurrentTroopCount);	
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

FBuildingResourceCost ATWPlayerState::GetTotalTroopUpkeepCost() const
{
	FBuildingResourceCost TotalCost;

	TotalCost.Wood = UpkeepCost.Wood * CurrentTroopCount;
	TotalCost.Ore = UpkeepCost.Ore * CurrentTroopCount;

	return TotalCost;
}

void ATWPlayerState::RefreshTroopUpkeepTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentTroopCount <= 0)
	{
		GetWorldTimerManager().ClearTimer(TroopUpkeepTimerHandle);

		for (TActorIterator<ATWTroopSpawnBuilding> It(GetWorld()); It; ++It)
		{
			ATWTroopSpawnBuilding* TroopBuilding = *It;
			if (!TroopBuilding)
			{
				continue;
			}

			if (TroopBuilding->GetOwnerPlayerState() != this)
			{
				continue;
			}

			TroopBuilding->SetQueuePausedByUpkeep(0);
		}

		return;
	}

	if (GetWorldTimerManager().IsTimerActive(TroopUpkeepTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		TroopUpkeepTimerHandle,
		this,
		&ATWPlayerState::HandleTroopUpkeep,
		UpkeepInterval,
		true
	);
}

void ATWPlayerState::HandleTroopUpkeep()
{
	if (!HasAuthority())
	{
		return;
	}

	const FBuildingResourceCost TotalCost = GetTotalTroopUpkeepCost();
	const int8 bPaidUpkeep = TrySpendTroopUpkeep();

	for (TActorIterator<ATWTroopSpawnBuilding> It(GetWorld()); It; ++It)
	{
		ATWTroopSpawnBuilding* TroopBuilding = *It;
		if (!TroopBuilding)
		{
			continue;
		}

		if (TroopBuilding->GetOwnerPlayerState() != this)
		{
			continue;
		}

		TroopBuilding->SetQueuePausedByUpkeep(bPaidUpkeep == 1 ? 0 : 1);
	}

	if (bPaidUpkeep == 1)
	{
		UE_LOG(LogTemp, Log, TEXT("PlayerSlot: %d | 유지비 지불 성공"), PlayerSlot);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayerSlot: %d | 유지비 부족 - 다음 체크까지 대기열 정지"), PlayerSlot);
	}
}

int8 ATWPlayerState::TrySpendTroopUpkeep()
{
	if (!HasAuthority())
	{
		return 0;
	}

	if (CurrentTroopCount <= 0)
	{
		return 1;
	}

	const FBuildingResourceCost TotalCost = GetTotalTroopUpkeepCost();

	if (CanAffordCost(TotalCost.Wood, TotalCost.Ore) == 0)
	{
		return 0;
	}

	SpendCost(TotalCost.Wood, TotalCost.Ore);
	return 1;
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
