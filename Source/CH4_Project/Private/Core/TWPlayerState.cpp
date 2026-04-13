#include "Core/TWPlayerState.h"
#include "Data/TWBuildingTypes.h"
#include "Net/UnrealNetwork.h"
#include "Building/TWTroopSpawnBuilding.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Core/TWPlayerController.h"

ATWPlayerState::ATWPlayerState()
{
	bReplicates = true;

	PlayerSlot = -1;

	Resources.SetNum(static_cast<int32>(EResourceType::Count));
	for (int32& Resource : Resources)
	{
		Resource = 0;
	}
	
	StatusUpgradeLevels.SetNumZeroed(static_cast<int32>(ETWStatusType::Count));
	
	PendingPopulation = 0;
	MaxPopulation = 200;
	PopulationLimit = 1;
	CurrentPopulation = 0;
}

void ATWPlayerState::SetPlayerSlot(const int32 InPlayerSlot)
{
	PlayerSlot = InPlayerSlot;

	UTWUnitSubsystem* UnitSubsystem = GetUnitSubsystem();
	if (IsValid(UnitSubsystem))
	{
		UnitSubsystem->AddPlayer(PlayerSlot);
	}
}

void ATWPlayerState::AddResource(const EResourceType ResourceType, const int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Amount <= 0 || ResourceType == EResourceType::Count)
	{
		return;
	}

	const int32 Index = static_cast<int32>(ResourceType);
	if (!Resources.IsValidIndex(Index))
	{
		return;
	}

	Resources[Index] += Amount;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Player %d | Wood: %d | Ore: %d"),
		PlayerSlot,
		GetResourceAmount(EResourceType::Wood),
		GetResourceAmount(EResourceType::Ore)
	);

	NotifyUIResourceStateChanged();
}

int8 ATWPlayerState::CanAffordCost(const TMap<EResourceType, int32>& Cost) const
{
	for (const TPair<EResourceType, int32>& Pair : Cost)
	{
		const int32 Index = static_cast<int32>(Pair.Key);
		if (!Resources.IsValidIndex(Index))
		{
			return 0;
		}

		if (Resources[Index] < Pair.Value)
		{
			return 0;
		}
	}

	return 1;
}

void ATWPlayerState::SpendCost(const TMap<EResourceType, int32>& Cost)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CanAffordCost(Cost) == 0)
	{
		return;
	}

	for (const TPair<EResourceType, int32>& Pair : Cost)
	{
		const int32 Index = static_cast<int32>(Pair.Key);
		if (!Resources.IsValidIndex(Index))
		{
			continue;
		}

		Resources[Index] -= Pair.Value;
		Resources[Index] = FMath::Max(0, Resources[Index]);
	}

	NotifyUIResourceStateChanged();
}

int8 ATWPlayerState::CanQueueTroop(const int32 RequiredPopulation) const
{
	if (RequiredPopulation <= 0)
	{
		return 0;
	}

	return ((CurrentPopulation + PendingPopulation + RequiredPopulation) <= PopulationLimit) ? 1 : 0;
}

void ATWPlayerState::SetCurrentPopulationFromContainer(const int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentPopulation = FMath::Max(0, InAmount);
	RefreshTroopUpkeepTimer();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerSlot: %d | CurrentPopulation(Container): %d / %d (Max: %d)"),
		PlayerSlot,
		CurrentPopulation,
		PopulationLimit,
		MaxPopulation
	);

	NotifyUIResourceStateChanged();
}

void ATWPlayerState::AddPendingPopulation(const int32 InAmount)
{
	if (!HasAuthority() || InAmount <= 0)
	{
		return;
	}

	PendingPopulation += InAmount;
	NotifyUIResourceStateChanged();
}

void ATWPlayerState::RemovePendingPopulation(const int32 InAmount)
{
	if (!HasAuthority() || InAmount <= 0)
	{
		return;
	}

	PendingPopulation -= InAmount;
	PendingPopulation = FMath::Max(0, PendingPopulation);

	NotifyUIResourceStateChanged();
}

void ATWPlayerState::AddPopulationLimit(const int32 InAmount)
{
	if (!HasAuthority() || InAmount <= 0)
	{
		return;
	}

	PopulationLimit += InAmount;
	PopulationLimit = FMath::Min(PopulationLimit, MaxPopulation);
	PopulationLimit = FMath::Max(1, PopulationLimit);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("PlayerSlot: %d | PopulationLimit: %d / %d"),
		PlayerSlot,
		PopulationLimit,
		MaxPopulation
	);

	NotifyUIResourceStateChanged();
}

void ATWPlayerState::RefreshTroopUpkeepTimer()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CurrentPopulation <= 0)
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

			TroopBuilding->SetQueuePausedByUpkeep(false);
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

		TroopBuilding->SetQueuePausedByUpkeep(bPaidUpkeep == 0);
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

	if (CurrentPopulation <= 0)
	{
		return 1;
	}

	const TMap<EResourceType, int32>& TotalCost = GetTotalTroopUpkeepCost();

	if (CanAffordCost(TotalCost) == 0)
	{
		return 0;
	}

	SpendCost(TotalCost);
	return 1;
}

int32 ATWPlayerState::GetStatusUpgradeLevel(const ETWStatusType StatusType) const
{
	const int32 Index = static_cast<int32>(StatusType);

	if (!StatusUpgradeLevels.IsValidIndex(Index))
	{
		return 0;
	}

	return StatusUpgradeLevels[Index];
}

void ATWPlayerState::AddStatusUpgradeLevel(const ETWStatusType StatusType, const int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InAmount <= 0)
	{
		return;
	}

	const int32 Index = static_cast<int32>(StatusType);

	if (!StatusUpgradeLevels.IsValidIndex(Index))
	{
		return;
	}

	StatusUpgradeLevels[Index] += InAmount;
}

void ATWPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWPlayerState, PlayerSlot);
	DOREPLIFETIME(ATWPlayerState, Resources);
	DOREPLIFETIME(ATWPlayerState, PendingPopulation);
	DOREPLIFETIME(ATWPlayerState, MaxPopulation);
	DOREPLIFETIME(ATWPlayerState, PopulationLimit);
	DOREPLIFETIME(ATWPlayerState, CurrentPopulation);
	DOREPLIFETIME(ATWPlayerState, StatusUpgradeLevels);
}

void ATWPlayerState::NotifyUIResourceStateChanged()
{
	AController* OwningController = GetOwningController();
	if (!OwningController)
	{
		return;
	}

	if (ATWPlayerController* TWPC = Cast<ATWPlayerController>(OwningController))
	{
		TWPC->NotifyResourceStateChanged();
	}
}

void ATWPlayerState::OnRep_Resources()
{
	NotifyUIResourceStateChanged();
}

void ATWPlayerState::OnRep_PendingPopulation()
{
	NotifyUIResourceStateChanged();
}

void ATWPlayerState::OnRep_PopulationLimit()
{
	NotifyUIResourceStateChanged();
}

void ATWPlayerState::OnRep_CurrentPopulation()
{
	NotifyUIResourceStateChanged();
}