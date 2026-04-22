#include "Subsystems/TWBuildingManagerSubsystem.h"
#include "Building/TWBaseBuilding.h"
#include "Log/TWLogCategory.h"

void UTWBuildingManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTWBuildingManagerSubsystem::Deinitialize()
{
	WorldBuildingMap.Empty();
	
	Super::Deinitialize();
}

void UTWBuildingManagerSubsystem::RegisterBuilding(int32 PlayerSlot, ATWBaseBuilding* Building)
{
	if (!Building)
	{
		return;
	}
	
	WorldBuildingMap.FindOrAdd(PlayerSlot).Buildings.AddUnique(Building);
	UE_LOG(LogTWSubsystem, Warning, TEXT("Registering Building | ID: %d, Total: %d"), PlayerSlot, WorldBuildingMap[PlayerSlot].Buildings.Num());
}

void UTWBuildingManagerSubsystem::UnregisterBuilding(int32 PlayerSlot, ATWBaseBuilding* Building)
{
	if (!Building)
	{
		return;
	}
	
	if (WorldBuildingMap.Contains(PlayerSlot))
	{
		WorldBuildingMap[PlayerSlot].Buildings.RemoveSingleSwap(Building);
		UE_LOG(LogTWSubsystem, Warning, TEXT("Unregistering Building | ID: %d, Remain: %d"), PlayerSlot, WorldBuildingMap[PlayerSlot].Buildings.Num());
	}
}

TArray<ATWBaseBuilding*> UTWBuildingManagerSubsystem::GetWorldBuildingsByPlayer(int32 PlayerSlot) const
{
	if (const FTWPlayerBuildingData* FoundData = WorldBuildingMap.Find(PlayerSlot))
	{
		return FoundData->Buildings;
	}
	
	return TArray<ATWBaseBuilding*>();
}

TArray<ATWBaseBuilding*> UTWBuildingManagerSubsystem::GetAllBuildings() const
{
	TArray<ATWBaseBuilding*> AllBuildings;
	
	for (const auto& Pair : WorldBuildingMap)
	{
		AllBuildings.Append(Pair.Value.Buildings);
	}
	return AllBuildings;
}
