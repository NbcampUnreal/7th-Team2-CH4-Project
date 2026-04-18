#include "Building/TWResourceBuilding.h"

#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Data/TWResourceBuildingDataAsset.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UI/World/TWFloatingTextActor.h"

const UTWResourceBuildingDataAsset* ATWResourceBuilding::GetResourceBuildingData() const
{
	return Cast<UTWResourceBuildingDataAsset>(BuildingData);
}

void ATWResourceBuilding::OnOwnerPlayerStateAssigned()
{
	if (!HasAuthority())
	{
		return;
	}

	StartResourceProduction();
}

void ATWResourceBuilding::StartResourceProduction()
{
	if (!HasAuthority())
	{
		return;
	}

	const UTWResourceBuildingDataAsset* ResourceData = GetResourceBuildingData();
	if (!ResourceData || !OwningPlayerState)
	{
		return;
	}

	if (ResourceData->ProducedResourceType == EResourceType::None)
	{
		return;
	}

	if (ResourceData->ProductionInterval <= 0.0f)
	{
		return;
	}

	ClearAllBuildingTimers();

	GetWorldTimerManager().SetTimer(
		ResourceProductionTimerHandle,
		this,
		&ATWResourceBuilding::HandleProduceResource,
		ResourceData->ProductionInterval,
		true
	);
}

void ATWResourceBuilding::HandleProduceResource()
{
	if (!HasAuthority())
	{
		return;
	}

	if (BuildingState != ETWBuildingState::Completed)
	{
		return;
	}

	const UTWResourceBuildingDataAsset* ResourceData = GetResourceBuildingData();
	if (!ResourceData || !OwningPlayerState)
	{
		return;
	}

	OwningPlayerState->AddResource(
		ResourceData->ProducedResourceType,
		ResourceData->ProductionAmount
	);

	Multicast_ShowResourceText(
		ResourceData->ProducedResourceType,
		ResourceData->ProductionAmount
	);
}

void ATWResourceBuilding::Multicast_ShowResourceText_Implementation(EResourceType InResourceType, int32 InAmount)
{
	if (!ShouldShowResourceTextOnThisClient())
	{
		return;
	}

	SpawnResourceText(InResourceType, InAmount);
}

void ATWResourceBuilding::SpawnResourceText(EResourceType InResourceType, int32 InAmount)
{
	if (!FloatingTextActorClass || !GetWorld())
	{
		return;
	}

	FVector SpawnLocation = GetHPBarAnchorWorldLocation();
	SpawnLocation.Z += FloatingTextZOffset;
	SpawnLocation.X += FMath::FRandRange(-FloatingTextRandomXYOffset, FloatingTextRandomXYOffset);
	SpawnLocation.Y += FMath::FRandRange(-FloatingTextRandomXYOffset, FloatingTextRandomXYOffset);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATWFloatingTextActor* FloatingText = GetWorld()->SpawnActor<ATWFloatingTextActor>(
		FloatingTextActorClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!FloatingText)
	{
		return;
	}

	FloatingText->InitializeText(
		MakeResourceText(InResourceType, InAmount),
		GetResourceTextColor(InResourceType),
		FloatingTextLifetime,
		FloatingTextRiseSpeed
	);
}

bool ATWResourceBuilding::ShouldShowResourceTextOnThisClient() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	ATWPlayerController* LocalPC = Cast<ATWPlayerController>(World->GetFirstPlayerController());
	if (!LocalPC || !LocalPC->IsLocalController())
	{
		return false;
	}

	ATWPlayerState* LocalPS = LocalPC->GetPlayerState<ATWPlayerState>();
	if (!LocalPS)
	{
		return false;
	}

	return LocalPS == OwningPlayerState || LocalPS->GetPlayerId() == OwnerPlayerSlot;
}

FString ATWResourceBuilding::MakeResourceText(EResourceType InResourceType, int32 InAmount) const
{
	FString ResourceName = TEXT("Resource");

	switch (InResourceType)
	{
	case EResourceType::Wood:
		ResourceName = TEXT("Wood");
		break;

	case EResourceType::Ore:
		ResourceName = TEXT("Ore");
		break;

	case EResourceType::Mithril:
		ResourceName = TEXT("Mithril");
		break;

	case EResourceType::None:
	case EResourceType::Count:
	default:
		ResourceName = TEXT("Resource");
		break;
	}

	return FString::Printf(TEXT("%s + %d"), *ResourceName, InAmount);
}

FColor ATWResourceBuilding::GetResourceTextColor(EResourceType InResourceType) const
{
	switch (InResourceType)
	{
	case EResourceType::Wood:
		return FColor(90, 255, 90, 255);

	case EResourceType::Ore:
		return FColor(200, 200, 200, 255);

	case EResourceType::Mithril:
		return FColor(120, 220, 255, 255);

	case EResourceType::None:
	case EResourceType::Count:
	default:
		return FColor::White;
	}
}

void ATWResourceBuilding::ClearAllBuildingTimers()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ResourceProductionTimerHandle);
}