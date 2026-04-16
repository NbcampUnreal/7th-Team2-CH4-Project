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

	RebuildCachedUnitVisualStyles();
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

void UTWSelectionVisualManager::RemoveStaleSmoothedUnitLocations()
{
	TSet<FMassNetworkID> CurrentSelectionSet;
	for (const FMassNetworkID& NetId : SelectedUnitNetIds)
	{
		CurrentSelectionSet.Add(NetId);
	}

	for (auto It = SmoothedUnitLocations.CreateIterator(); It; ++It)
	{
		if (!CurrentSelectionSet.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}

void UTWSelectionVisualManager::RebuildCachedUnitVisualStyles()
{
	CachedUnitVisualStyles.Empty();

	if (!OwnerController)
	{
		return;
	}

	UWorld* World = OwnerController->GetWorld();
	if (!World)
	{
		return;
	}

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return;
	}

	for (const FMassNetworkID& NetId : SelectedUnitNetIds)
	{
		FTWUnitSelectionVisualStyle Style;
		if (UnitSubsystem->TryGetUnitSelectionVisualStyle(NetId, Style))
		{
			CachedUnitVisualStyles.Add(NetId, Style);
		}
	}
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

	RemoveStaleSmoothedUnitLocations();

	for (int32 Index = SelectedBuildings.Num() - 1; Index >= 0; --Index)
	{
		if (!SelectedBuildings[Index].IsValid())
		{
			SelectedBuildings.RemoveAt(Index);
		}
	}

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (UnitSubsystem)
	{
		const int32 DrawCount = FMath::Min(SelectedUnitNetIds.Num(), MaxDebugDrawUnitCount);

		for (int32 Index = 0; Index < DrawCount; ++Index)
		{
			const FMassNetworkID& NetId = SelectedUnitNetIds[Index];

			FVector TargetLocation = FVector::ZeroVector;
			if (!UnitSubsystem->TryGetUnitVisualLocation(NetId, TargetLocation))
			{
				continue;
			}

			FTWUnitSelectionVisualStyle Style;
			if (const FTWUnitSelectionVisualStyle* CachedStyle = CachedUnitVisualStyles.Find(NetId))
			{
				Style = *CachedStyle;
			}
			else
			{
				Style.SelectionCircleRadius = DefaultUnitSelectionCircleRadius;
				Style.CircleThickness = DefaultUnitCircleThickness;
				Style.LocationInterpSpeed = DefaultUnitLocationInterpSpeed;
				Style.LocationSnapDistance = DefaultUnitLocationSnapDistance;
				Style.LocationMaxInterpSpeedMultiplier = DefaultUnitLocationMaxInterpSpeedMultiplier;
			}

			FVector* SmoothedLocationPtr = SmoothedUnitLocations.Find(NetId);
			if (!SmoothedLocationPtr)
			{
				SmoothedUnitLocations.Add(NetId, TargetLocation);
				SmoothedLocationPtr = SmoothedUnitLocations.Find(NetId);
			}

			if (!SmoothedLocationPtr)
			{
				continue;
			}

			const float Distance = FVector::Dist(*SmoothedLocationPtr, TargetLocation);

			if (Distance >= Style.LocationSnapDistance)
			{
				*SmoothedLocationPtr = TargetLocation;
			}
			else
			{
				const float DynamicInterpSpeed = FMath::GetMappedRangeValueClamped(
					FVector2D(0.f, Style.LocationSnapDistance),
					FVector2D(
						Style.LocationInterpSpeed,
						Style.LocationInterpSpeed * Style.LocationMaxInterpSpeedMultiplier
					),
					Distance
				);

				*SmoothedLocationPtr = FMath::VInterpTo(
					*SmoothedLocationPtr,
					TargetLocation,
					DeltaSeconds,
					DynamicInterpSpeed
				);
			}

			const FVector RingLocation = *SmoothedLocationPtr;

			FVector HPBarLocation = FVector::ZeroVector;
			if (!UnitSubsystem->TryGetUnitHPBarWorldLocation(NetId, HPBarLocation))
			{
				HPBarLocation = *SmoothedLocationPtr + FVector(0.f, 0.f, 120.f);
			}

			DrawDebugCircle(
				World,
				RingLocation,
				Style.SelectionCircleRadius,
				24,
				FColor::Green,
				false,
				0.f,
				0,
				Style.CircleThickness,
				FVector(1.f, 0.f, 0.f),
				FVector(0.f, 1.f, 0.f),
				false
			);

			DrawDebugString(
				World,
				HPBarLocation,
				TEXT("Selected"),
				nullptr,
				FColor::White,
				0.f,
				false
			);
		}
	}

	for (const TWeakObjectPtr<ATWBaseBuilding>& BuildingPtr : SelectedBuildings)
	{
		if (!BuildingPtr.IsValid())
		{
			continue;
		}

		const FVector2D HalfExtentXY =
			BuildingPtr->GetSelectionRectangleHalfExtentXY(BuildingSelectionPadding);

		const float ZOffset =
			BuildingPtr->GetSelectionRectangleZOffset(BuildingSelectionZOffset);

		const FVector BoxCenter =
			BuildingPtr->GetActorLocation() + FVector(0.f, 0.f, ZOffset);

		const FVector DebugHalfExtent(
			HalfExtentXY.X,
			HalfExtentXY.Y,
			2.f
		);

		const FVector HPBarLocation = BuildingPtr->GetSelectionHPBarWorldLocation();

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

		DrawDebugString(
			World,
			HPBarLocation,
			FString::Printf(TEXT("HP %.0f / %.0f"), BuildingPtr->GetCurrentHP(), BuildingPtr->GetMaxHP()),
			nullptr,
			FColor::White,
			0.f,
			false
		);
	}
}