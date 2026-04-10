#include "Building/TWNexusBuilding.h"
#include "Data/TWNexusBuildingDataAsset.h"
#include "Core/TWGameMode.h"
#include "TimerManager.h"

void ATWNexusBuilding::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	StartHPRegen();
}

const UTWNexusBuildingDataAsset* ATWNexusBuilding::GetNexusBuildingData() const
{
	return Cast<UTWNexusBuildingDataAsset>(BuildingData);
}

void ATWNexusBuilding::ApplyDamageToBuilding(const int32 InDamageAmount)
{
	Super::ApplyDamageToBuilding(InDamageAmount);

	if (!HasAuthority())
	{
		return;
	}
	
	if (InDamageAmount <= 0)
	{
		return;
	}

	if (CurrentHP <= 0)
	{
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[Nexus] HP Damage : %d / %d"), CurrentHP, MaxHP);

	const UTWNexusBuildingDataAsset* NexusData = GetNexusBuildingData();
	if (!NexusData)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RegenDelayTimerHandle);
	GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);

	if (NexusData->RegenDelay <= 0.0f)
	{
		StartHPRegen();
		return;
	}

	GetWorldTimerManager().SetTimer(
		RegenDelayTimerHandle,
		this,
		&ATWNexusBuilding::StartHPRegen,
		NexusData->RegenDelay,
		false
	);
}

void ATWNexusBuilding::StartHPRegen()
{
	if (!HasAuthority())
	{
		return;
	}

	const UTWNexusBuildingDataAsset* NexusData = GetNexusBuildingData();
	if (!NexusData)
	{
		return;
	}

	if (CurrentHP <= 0)
	{
		return;
	}

	if (CurrentHP >= MaxHP)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		RegenTickTimerHandle,
		this,
		&ATWNexusBuilding::HandleHPRegen,
		NexusData->RegenInterval,
		true
	);
}

void ATWNexusBuilding::HandleHPRegen()
{
	if (!HasAuthority())
	{
		return;
	}

	const UTWNexusBuildingDataAsset* NexusData = GetNexusBuildingData();
	if (!NexusData)
	{
		return;
	}

	if (CurrentHP <= 0)
	{
		GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);
		return;
	}

	CurrentHP += NexusData->RegenAmount;
	CurrentHP = FMath::Min(CurrentHP, MaxHP);

	if (CurrentHP >= MaxHP)
	{
		GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);
	}
	
	UE_LOG(LogTemp, Log, TEXT("[Nexus] HP Regen : %d / %d"), CurrentHP, MaxHP);
}

void ATWNexusBuilding::HandleDestroyedByDamage()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearAllBuildingTimers();
	
	if (ATWGameMode* TWGameMode = GetWorld()->GetAuthGameMode<ATWGameMode>())
	{
		TWGameMode->HandlePlayerDefeat(OwnerPlayerSlot);
	}

	Destroy();
}

void ATWNexusBuilding::ClearAllBuildingTimers()
{
	Super::ClearAllBuildingTimers();
	GetWorldTimerManager().ClearTimer(RegenDelayTimerHandle);
	GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);
}