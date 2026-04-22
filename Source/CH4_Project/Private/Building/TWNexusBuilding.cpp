#include "Building/TWNexusBuilding.h"
#include "Data/TWNexusBuildingDataAsset.h"
#include "Core/TWGameMode.h"
#include "Core/TWPlayerState.h"
#include "Data/TWBuildingTypes.h"
#include "TimerManager.h"
void ATWNexusBuilding::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	StartHPRegen();

	if (OwningPlayerState)
	{
		StartWoodProduction();
	}
}

const UTWNexusBuildingDataAsset* ATWNexusBuilding::GetNexusBuildingData() const
{
	return Cast<UTWNexusBuildingDataAsset>(BuildingData);
}

void ATWNexusBuilding::OnOwnerPlayerStateAssigned()
{
	Super::OnOwnerPlayerStateAssigned();

	if (!HasAuthority())
	{
		return;
	}

	StartWoodProduction();
}

void ATWNexusBuilding::ApplyDamageToBuilding(const float InDamageAmount)
{
	Super::ApplyDamageToBuilding(InDamageAmount);

	if (!HasAuthority())
	{
		return;
	}

	if (InDamageAmount <= 0.0f)
	{
		return;
	}

	if (CurrentHP <= 0.0f)
	{
		return;
	}

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

	if (CurrentHP <= 0.0f)
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

	if (CurrentHP <= 0.0f)
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
}

void ATWNexusBuilding::StartWoodProduction()
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

	if (!OwningPlayerState)
	{
		return;
	}

	if (NexusData->WoodProductionInterval <= 0.0f)
	{
		return;
	}

	if (NexusData->WoodProductionAmount <= 0)
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(WoodProductionTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		WoodProductionTimerHandle,
		this,
		&ATWNexusBuilding::HandleWoodProduction,
		NexusData->WoodProductionInterval,
		true
	);
}

void ATWNexusBuilding::HandleWoodProduction()
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

	if (!OwningPlayerState)
	{
		return;
	}

	if (CurrentHP <= 0.0f)
	{
		return;
	}

	OwningPlayerState->AddResource(EResourceType::Wood, NexusData->WoodProductionAmount);
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
	GetWorldTimerManager().ClearTimer(WoodProductionTimerHandle);
}

FVector ATWNexusBuilding::GetHeroSpawnLocation(float ForwardDistance) const
{
	return GetActorLocation() + (GetActorForwardVector() * ForwardDistance);
}