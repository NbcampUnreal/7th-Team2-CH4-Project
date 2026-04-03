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
	QueryParams.bTraceComplex = true;
	
	ECollisionChannel LandscapeChannel = ECC_GameTraceChannel1;
	
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
