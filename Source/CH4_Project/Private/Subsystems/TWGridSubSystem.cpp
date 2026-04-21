// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/TWGridSubSystem.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Landscape.h"
#include "EngineUtils.h"
#include "Log/TWLogCategory.h"
#include "FOW/TWFogManager.h"

void UTWGridSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

}

void UTWGridSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	ALandscape* FoundLandscape = nullptr;
	
	for (TActorIterator<ALandscape> It(&InWorld); It; ++It)
	{
		FoundLandscape = *It;
		break;
	}
	
	if (FoundLandscape)
	{
		FBox LandscapeBounds = FoundLandscape->GetComponentsBoundingBox();
		
		GridOrigin = FVector(LandscapeBounds.Min.X, LandscapeBounds.Min.Y, 0.0f);
		
		float SizeX = LandscapeBounds.Max.X - LandscapeBounds.Min.X;
		float SizeY = LandscapeBounds.Max.Y - LandscapeBounds.Min.Y;
		
		GridDimensions.X = FMath::CeilToInt(SizeX / CellSize);
		GridDimensions.Y = FMath::CeilToInt(SizeY / CellSize);
		
		UE_LOG(LogNT, Warning, TEXT("Origin: %s, Dimensions: %s"), 
			*GridOrigin.ToString(), *GridDimensions.ToString());
	}
	else
	{
		UE_LOG(LogNT, Warning, TEXT("Landscape x"));
		GridOrigin = FVector::ZeroVector;
		GridDimensions = FIntPoint(200, 200);
	}
	
		
	int32 TotalCells = GridDimensions.X * GridDimensions.Y;
	GridData.Empty();
	GridData.SetNum(TotalCells);
	
	UE_LOG(LogNT, Warning, TEXT("TotalCells: %d"), TotalCells);
	
	FCollisionQueryParams QueryParams;	
	QueryParams.bTraceComplex = true;
	
	ECollisionChannel LandscapeChannel = ECC_GameTraceChannel1;
	ECollisionChannel WaterChannel = ECC_GameTraceChannel3;
	
	for (int32 Y = 0; Y < GridDimensions.Y; Y++)
	{
		for (int32 X = 0; X < GridDimensions.X; X++)
		{
			float WorldX = GridOrigin.X + (X * CellSize) + (CellSize * 0.5f);
			float WorldY = GridOrigin.Y + (Y * CellSize) + (CellSize * 0.5f);
			
			FVector TraceStart(WorldX, WorldY, 10000.0f);
			FVector TraceEnd(WorldX, WorldY, -10000.0f);
			
			FHitResult WaterHitResult;
			FHitResult LandscapeHitResult;
			
			bool bHitWater = InWorld.LineTraceSingleByChannel(
					WaterHitResult
					, TraceStart
					, TraceEnd
					, WaterChannel
					, QueryParams
			);
			
			bool bHitLandscape = InWorld.LineTraceSingleByChannel(
					LandscapeHitResult
					, TraceStart
					, TraceEnd
					, LandscapeChannel
					, QueryParams
			);
			
			if (bHitWater && bHitLandscape)
			{
				if (LandscapeHitResult.ImpactPoint.Z > WaterHitResult.ImpactPoint.Z)
				{
					bHitWater = false;
				}
			}
			
			int32 Index = (Y * GridDimensions.X) + X;
			
			if (bHitWater)
			{
				GridData[Index].TerrarianZ = WaterHitResult.ImpactPoint.Z;
				GridData[Index].bIsBuildable = false;
				
				DrawDebugPoint(&InWorld, WaterHitResult.ImpactPoint, 10.0f, FColor::Blue, true);
			}
			else if (bHitLandscape)
			{
				GridData[Index].TerrarianZ = LandscapeHitResult.ImpactPoint.Z;
				GridData[Index].bIsBuildable = true;
				
				DrawDebugPoint(&InWorld, LandscapeHitResult.ImpactPoint, 10.0f, FColor::Green, true);
			}
			else
			{
				GridData[Index].TerrarianZ = 0.0f;
				GridData[Index].bIsBuildable = false;
				
				DrawDebugLine(&InWorld, TraceStart, TraceEnd, FColor::Red, true);
			}
		}
	}
	UE_LOG(LogNT, Warning, TEXT("Z축 캐싱 완료"));
}

FIntPoint UTWGridSubSystem::WorldToGridPosition(const FVector& WorldLocation) const
{
	int32 X = FMath::FloorToInt((WorldLocation.X - GridOrigin.X) / CellSize);
	int32 Y = FMath::FloorToInt((WorldLocation.Y - GridOrigin.Y) / CellSize);
	
	return FIntPoint(X, Y);
}

FVector UTWGridSubSystem::GridToWorldPosition(const FIntPoint& GridLocation) const
{
	float WorldX = GridOrigin.X + (GridLocation.X * CellSize) + (CellSize * 0.5f);
	float WorldY = GridOrigin.Y + (GridLocation.Y * CellSize) + (CellSize * 0.5f);
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
	// 안개 매니저를 찾아 시야 확인 준비
	ATWFogManager* FogManager = nullptr;
	for (TActorIterator<ATWFogManager> It(GetWorld()); It; ++It)
	{
		if (ATWFogManager* FoundActor = *It)
		{
			FogManager = FoundActor;
			break;
		}
	}
	
	for (int32 Y = 0; Y < BuildingSize.Y; Y++)
	{
		for (int32 X = 0; X < BuildingSize.X; X++)
		{
			FIntPoint TargetGrid(AnchorLocation.X + X, AnchorLocation.Y + Y);
			
			if (!CanBuildAt(TargetGrid))
			{
				return false;
			}
			
			// 건설하려는 그리드 지점이 안개 속인지 검사
			if (FogManager)
			{
				FVector GridWorldPos = GridToWorldPosition(TargetGrid);
				if (!FogManager->IsLocationVisible(GridWorldPos))
				{
					return false;
				}
			}
		}
	}
	
	UWorld* World = GetWorld();
	
	FVector CenterLocation = GetBuildingCenterPosition(AnchorLocation, BuildingSize);
	
	CenterLocation.Z += 50.0f;
	
	FVector BoxExtent(BuildingSize.X * CellSize * 0.45f, BuildingSize.Y * CellSize * 0.45f, 50.0f);
	
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	
	bool bIsOverlapping = World->OverlapAnyTestByChannel(
		CenterLocation,
		FQuat::Identity,
		ECC_GameTraceChannel2,
		FCollisionShape::MakeBox(BoxExtent),
		QueryParams
	);
	
	FColor BoxColor = bIsOverlapping ? FColor::Red : FColor::Green;
	DrawDebugBox(World, CenterLocation, BoxExtent, BoxColor, false, -1.0f, 0, 5.0f);
	
	if (bIsOverlapping)
	{
		return false;
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
	float WorldX = GridOrigin.X + (AnchorLocation.X * CellSize) + (BuildingSize.X * CellSize * 0.5f);
	float WorldY = GridOrigin.Y + (AnchorLocation.Y * CellSize) + (BuildingSize.Y * CellSize * 0.5f);
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
