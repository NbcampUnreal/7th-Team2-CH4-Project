#include "Selection/TWSelectionVisualManager.h"

#include "Core/TWPlayerController.h"
#include "Building/TWBaseBuilding.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

void UTWSelectionVisualManager::Initialize(ATWPlayerController* InOwnerController)
{
	OwnerController = InOwnerController;
	ClearSelectionVisuals();
}

void UTWSelectionVisualManager::ApplyUnitSelection(const TArray<FMassNetworkID>& InSelectedUnits)
{
	for (const TWeakObjectPtr<ATWBaseBuilding>& BuildingPtr : SelectedBuildings)
	{
		if (BuildingPtr.IsValid())
		{
			BuildingPtr->SetSelectionVisualActive(false);
		}
	}

	SelectedBuildings.Empty();
	SelectedUnitNetIds = InSelectedUnits;

	TMap<FMassNetworkID, FVector> NewSmoothedLocations;
	for (const FMassNetworkID& NetId : SelectedUnitNetIds)
	{
		if (const FVector* FoundLocation = SmoothedUnitLocations.Find(NetId))
		{
			NewSmoothedLocations.Add(NetId, *FoundLocation);
		}
	}

	SmoothedUnitLocations = MoveTemp(NewSmoothedLocations);
}

void UTWSelectionVisualManager::ApplyBuildingSelection(const TArray<ATWBaseBuilding*>& InSelectedBuildings)
{
	for (const TWeakObjectPtr<ATWBaseBuilding>& BuildingPtr : SelectedBuildings)
	{
		if (BuildingPtr.IsValid())
		{
			BuildingPtr->SetSelectionVisualActive(false);
		}
	}

	SelectedUnitNetIds.Empty();
	SmoothedUnitLocations.Empty();
	CachedUnitVisualStyles.Empty();
	SelectedBuildings.Empty();

	for (ATWBaseBuilding* Building : InSelectedBuildings)
	{
		if (!IsValid(Building))
		{
			continue;
		}

		Building->SetSelectionVisualActive(true);
		SelectedBuildings.Add(Building);
	}
}

void UTWSelectionVisualManager::ClearSelectionVisuals()
{
	for (const TWeakObjectPtr<ATWBaseBuilding>& BuildingPtr : SelectedBuildings)
	{
		if (BuildingPtr.IsValid())
		{
			BuildingPtr->SetSelectionVisualActive(false);
		}
	}

	SelectedUnitNetIds.Empty();
	SelectedBuildings.Empty();
	SmoothedUnitLocations.Empty();
	CachedUnitVisualStyles.Empty();
}

void UTWSelectionVisualManager::Tick(float DeltaSeconds)
{
	if (!OwnerController || !bEnableDebugDraw)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	const int32 DrawCount = FMath::Min(SelectedUnitRingVisuals.Num(), MaxDebugDrawUnitCount);
	for (int32 Index = 0; Index < DrawCount; ++Index)
	{
		const FTWUnitRingVisualData& RingData = SelectedUnitRingVisuals[Index];

		DrawDebugCircle(
			World,
			RingData.RingWorldLocation,
			RingData.RingRadius,
			24,
			FColor::Green,
			false,
			0.f,
			0,
			DefaultUnitCircleThickness,
			FVector(1.f, 0.f, 0.f),
			FVector(0.f, 1.f, 0.f),
			false
		);
	}

	if (!PrimarySelectedVisualData.bValid)
	{
		return;
	}

	if (PrimarySelectedVisualData.bIsBuilding)
	{
		const FVector BoxCenter =
			PrimarySelectedVisualData.SelectionWorldLocation +
			FVector(0.f, 0.f, PrimarySelectedVisualData.BuildingZOffset);

		const FVector DebugHalfExtent(
			PrimarySelectedVisualData.BuildingHalfExtentXY.X,
			PrimarySelectedVisualData.BuildingHalfExtentXY.Y,
			2.f
		);

		DrawDebugBox(
			World,
			BoxCenter,
			DebugHalfExtent,
			FQuat::Identity,
			FColor::Yellow,
			false,
			0.f,
			0,
			BuildingSelectionLineThickness
		);
	}
	else
	{
		DrawDebugCircle(
			World,
			PrimarySelectedVisualData.SelectionWorldLocation,
			PrimarySelectedVisualData.SelectionRadius,
			24,
			FColor::Green,
			false,
			0.f,
			0,
			DefaultUnitCircleThickness,
			FVector(1.f, 0.f, 0.f),
			FVector(0.f, 1.f, 0.f),
			false
		);
	}

	DrawDebugString(
		World,
		PrimarySelectedVisualData.HPBarWorldLocation,
		FString::Printf(
			TEXT("HP %.0f / %.0f (%.0f%%)"),
			PrimarySelectedVisualData.CurrentHP,
			PrimarySelectedVisualData.MaxHP,
			PrimarySelectedVisualData.HealthPercent * 100.f
		),
		nullptr,
		FColor::White,
		0.f,
		false
	);
}

void UTWSelectionVisualManager::SetPrimarySelectedVisualData(const FTWSelectedVisualData& InData)
{
	PrimarySelectedVisualData = InData;
}

void UTWSelectionVisualManager::SetSelectedUnitRingVisuals(const TArray<FTWUnitRingVisualData>& InData)
{
	SelectedUnitRingVisuals = InData;
}