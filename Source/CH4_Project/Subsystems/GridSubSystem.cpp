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

void UGridSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	FCollisionQueryParams QueryParams;	
	QueryParams.bTraceComplex = false;
	
	for (int32 Y = 0; Y < GridDimensions.Y; Y++)
	{
		for (int32 X = 0; X < GridDimensions.X; X++)
		{
			float WorldX = (X * CellSize) + (CellSize * 0.5f);
			float WorldY = (Y * CellSize) + (CellSize * 0.5f);
			
			FVector TraceStart(WorldX, WorldY, 10000.0f);
			FVector TraceEnd(WorldX, WorldY, -10000.0f);
			FHitResult HitResult;
			
			bool bHit = InWorld.LineTraceSingleByChannel(
					HitResult
					, TraceStart
					, TraceEnd
					, ECC_Visibility
					, QueryParams
			);
			
			int32 Index = (Y * GridDimensions.X) + X;
			
			if (bHit)
			{
				GridData[Index].TerrarianZ = HitResult.ImpactPoint.Z;
			}
			else
			{
				GridData[Index].TerrarianZ = 0.0f;
				GridData[Index].bIsBuildable = false;
			}
		}
	}
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
	float CachedZ = 0.0f;
	
	if (IsValidGridLocation(GridLocation))
	{
		int32 Index = (GridLocation.Y * GridDimensions.X)+GridLocation.X;
		CachedZ = GridData[Index].TerrarianZ;
	}
	
	return FVector(WorldX, WorldY, CachedZ);
}

bool UGridSubSystem::IsValidGridLocation(const FIntPoint& GridLocation) const
{
	return	GridLocation.X >= 0 && GridLocation.X < GridDimensions.X &&
			GridLocation.Y >= 0 && GridLocation.Y < GridDimensions.Y;
}
