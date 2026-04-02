#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridCellData.h"
#include "GridSubSystem.generated.h"


UCLASS()
class CH4_PROJECT_API UGridSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Coordinate")
	FIntPoint WorldToGridPosition(const FVector& WorldLocation) const;	// 월드 3D 좌표 -> 논리 2D 그리드 좌표
	UFUNCTION(BlueprintCallable, Category = "Grid|Coordinate")
	FVector GridToWorldPosition(const FIntPoint& GridLocation) const;	// 논리 2D 그리드 좌표 -> 월드 3D 좌표
	
	
	
	// UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	// bool CanBuildAt(const FIntPoint& GridLocation) const;	// 좌표에 건물을 건설할 수 있는지 검사
	// UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	// void PlaceBuildIngAt(const FIntPoint& GridLocation, AActor* Building);	// 좌표에 건물을 건설하고 데이터 업데이트
	//
	// UFUNCTION(BlueprintCallable, Category = "Grid|FogOfWar")
	// void UpdateVisionAroundLocation(const FIntPoint& CenterLocation, int32 VisionRadius);	// 건물 주변 반경 시야 Visible로 갱신
	//
private:
	UPROPERTY(EditDefaultsOnly, Category="Grid|Settings")
	float CellSize = 100.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Grid|Settings")
	FIntPoint GridDimensions = FIntPoint(100, 100);
	
	TArray<FGridCellData> GridData;	
	
	bool IsValidGridLocation(const FIntPoint& GridLocation) const;	// 그리드 좌표 유효 범위 확인
};
