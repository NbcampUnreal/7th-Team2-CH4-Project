// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/TWGridSubSystem.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

void UTWGridSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	int32 TotalCells = GridDimensions.X * GridDimensions.Y;
	GridData.SetNum(TotalCells);
	
	UE_LOG(LogTemp, Warning, TEXT("TotalCells: %d"), TotalCells);
}

void UTWGridSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	FCollisionQueryParams QueryParams;	
	QueryParams.bTraceComplex = true;
	
	ECollisionChannel LandscapeChannel = ECC_GameTraceChannel1;
	
	for (int32 Y = 0; Y < GridDimensions.Y; Y++)
	{
		for (int32 X = 0; X < GridDimensions.X; X++)
		{
			float WorldX = (X * CellSize) + (CellSize * 0.5f);// + GridOrigin.X;
			float WorldY = (Y * CellSize) + (CellSize * 0.5f);// + GridOrigin.Y;
			
			FVector TraceStart(WorldX, WorldY, 10000.0f);
			FVector TraceEnd(WorldX, WorldY, -10000.0f);
			FHitResult HitResult;
			
			bool bHit = InWorld.LineTraceSingleByChannel(
					HitResult
					, TraceStart
					, TraceEnd
					, LandscapeChannel
					, QueryParams
			);
			
			int32 Index = (Y * GridDimensions.X) + X;
			
			if (bHit)
			{
				GridData[Index].TerrarianZ = HitResult.ImpactPoint.Z;
				
				DrawDebugPoint(&InWorld, HitResult.ImpactPoint, 10.0f, FColor::Green, true);
			}
			else
			{
				GridData[Index].TerrarianZ = 0.0f;
				GridData[Index].bIsBuildable = false;
				
				DrawDebugLine(&InWorld, TraceStart, TraceEnd, FColor::Red, true);
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Z축 캐싱 완료"));
}

FIntPoint UTWGridSubSystem::WorldToGridPosition(const FVector& WorldLocation) const
{
	int32 X = FMath::FloorToInt(WorldLocation.X / CellSize);
	int32 Y = FMath::FloorToInt(WorldLocation.Y / CellSize);
	
	return FIntPoint(X, Y);
}

FVector UTWGridSubSystem::GridToWorldPosition(const FIntPoint& GridLocation) const
{
	float WorldX = (GridLocation.X * CellSize) + (CellSize * 0.5f);// + GridOrigin.X;
	float WorldY = (GridLocation.Y * CellSize) + (CellSize * 0.5f);// + GridOrigin.Y;
	float CachedZ = 0.0f;
	
	if (IsValidGridLocation(GridLocation))
	{
		int32 Index = (GridLocation.Y * GridDimensions.X)+GridLocation.X;
		CachedZ = GridData[Index].TerrarianZ;
	}
	
	return FVector(WorldX, WorldY, CachedZ);
}

bool UTWGridSubSystem::CanBuildArea(FIntPoint AnchorLocation, FIntPoint BuildingSize) const
{
	for (int32 Y = 0; Y < BuildingSize.Y; Y++)
	{
		for (int32 X = 0; X < BuildingSize.X; X++)
		{
			FIntPoint TargetGrid(AnchorLocation.X + X, AnchorLocation.Y + Y);
			
			if (!CanBuildAt(TargetGrid))
			{
				return false;
			}
		}
	}
	return true;
}

void UTWGridSubSystem::OccupyArea(FIntPoint AnchorLocation, FIntPoint BuildingSize, AActor* BuildingAnchor)
{
	for (int32 Y = 0; Y < BuildingSize.Y; ++Y)
	{
		for (int32 X = 0; X < BuildingSize.X; ++X)
		{
			FIntPoint TargetGrid(AnchorLocation.X + X, AnchorLocation.Y + Y);
			
			PlaceBuildingAt(TargetGrid, BuildingAnchor);
		}
	}
}

void UTWGridSubSystem::FreeArea(FIntPoint AnchorLocation, FIntPoint BuildingSize)
{
	for (int32 Y = 0; Y < BuildingSize.Y; ++Y)
	{
		for (int32 X = 0; X < BuildingSize.X; ++X)
		{
			FIntPoint TargetGrid(AnchorLocation.X + X, AnchorLocation.Y + Y);
			
			RemoveBuildingAt(TargetGrid);
		}
	}
}

FVector UTWGridSubSystem::GetBuildingCenterPosition(FIntPoint AnchorLocation, FIntPoint BuildingSize) const
{
	float WorldX = (AnchorLocation.X * CellSize) + (BuildingSize.X * CellSize *0.5f);// + GridOrigin.X;
	float WorldY = (AnchorLocation.Y * CellSize) + (BuildingSize.Y * CellSize *0.5f);// + GridOrigin.Y;
	float CachedZ = 0.0f;
	
	if (IsValidGridLocation(AnchorLocation))
	{
		int32 Index = (AnchorLocation.Y * GridDimensions.X) + AnchorLocation.X;
		CachedZ = GridData[Index].TerrarianZ;
	}
	
	return FVector(WorldX, WorldY, CachedZ);
}

bool UTWGridSubSystem::CanBuildAt(const FIntPoint& GridLocation) const
{
	if (!IsValidGridLocation(GridLocation))
	{
		return false;
	}
	
	int32 Index = (GridLocation.Y * GridDimensions.X) + GridLocation.X;
	
	if (!GridData[Index].bIsBuildable || GridData[Index].OccupyingBuilding != nullptr)
	{
		return false;
	}
	
	return true;
}

void UTWGridSubSystem::PlaceBuildingAt(const FIntPoint& GridLocation, AActor* Building)
{
	if (IsValidGridLocation(GridLocation))
	{
		int32 Index = (GridLocation.Y * GridDimensions.X) + GridLocation.X;
		
		GridData[Index].OccupyingBuilding = Building;
		GridData[Index].bIsBuildable = false;
	}
}

void UTWGridSubSystem::RemoveBuildingAt(const FIntPoint& GridLocation)
{
	if (IsValidGridLocation(GridLocation))
	{
		int32 Index = (GridLocation.Y * GridDimensions.X) + GridLocation.X;
		
		GridData[Index].OccupyingBuilding = nullptr;
		GridData[Index].bIsBuildable = true;
	}
}

bool UTWGridSubSystem::IsValidGridLocation(const FIntPoint& GridLocation) const
{
	return	GridLocation.X >= 0 && GridLocation.X < GridDimensions.X &&
			GridLocation.Y >= 0 && GridLocation.Y < GridDimensions.Y;
}
