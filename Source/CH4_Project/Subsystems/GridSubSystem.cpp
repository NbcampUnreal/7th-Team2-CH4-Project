// Fill out your copyright notice in the Description page of Project Settings.


#include "GridSubSystem.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

void UGridSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	int32 TotalCells = GridDimensions.X * GridDimensions.Y;
	GridData.SetNum(TotalCells);
	
	UE_LOG(LogTemp, Warning, TEXT("TotalCells: %d"), TotalCells);
}

FIntPoint UGridSubSystem::WorldToGridPosition(const FVector& WorldLocation) const
{
	int32 X = FMath::FloorToInt(WorldLocation.X / CellSize);
	int32 Y = FMath::FloorToInt(WorldLocation.Y / CellSize);
	
	return FIntPoint(X, Y);
}

FVector UGridSubSystem::GridToWorldPosition(const FIntPoint& GridLocation) const
{
	float WorldX = (GridLocation.X * CellSize) + (CellSize * 0.5f);
	float WorldY = (GridLocation.Y * CellSize) + (CellSize * 0.5f);
	
	return FVector(WorldX, WorldY, 0.0f);
}

bool UGridSubSystem::IsValidGridLocation(const FIntPoint& GridLocation) const
{
	return	GridLocation.X >= 0 && GridLocation.X < GridDimensions.X &&
			GridLocation.Y >= 0 && GridLocation.Y < GridDimensions.Y;
}
