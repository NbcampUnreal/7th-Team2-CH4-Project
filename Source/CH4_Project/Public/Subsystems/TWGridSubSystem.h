#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TWGridCellData.h"
#include "TWGridSubSystem.generated.h"


UCLASS()
class CH4_PROJECT_API UTWGridSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
#pragma region Grid
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Coordinate")
	FIntPoint WorldToGridPosition(const FVector& WorldLocation) const;	// 월드 3D 좌표 -> 논리 2D 그리드 좌표
	UFUNCTION(BlueprintCallable, Category = "Grid|Coordinate")
	FVector GridToWorldPosition(const FIntPoint& GridLocation) const;	// 논리 2D 그리드 좌표 -> 월드 3D 좌표
	
#pragma endregion
	
#pragma region Util
	
	UFUNCTION(BlueprintPure, Category = "Grid|Settings")
	float GetCellSize() const { return CellSize; }
	UFUNCTION(BlueprintPure, Category = "Grid|Settings")
	FIntPoint GetGridDimensions() const { return GridDimensions; }
	
    UFUNCTION(BlueprintPure, Category = "Grid|Settings")
    FVector GetGridOrigin() const { return GridOrigin; }

    UFUNCTION(BlueprintPure, Category = "Grid|Settings")
    FVector2D GetGridFullSize() const { return FVector2D(GridDimensions.X * CellSize, GridDimensions.Y * CellSize); }
#pragma endregion 
	
	
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	bool CanBuildArea(FIntPoint AnchorLocation, FIntPoint BuildingSize) const;
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	void OccupyArea(FIntPoint AnchorLocation, FIntPoint BuildingSize, AActor* BuildingAnchor);
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	void FreeArea(FIntPoint AnchorLocation, FIntPoint BuildingSize);
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Coordinate")
	FVector GetBuildingCenterPosition(FIntPoint AnchorLocation, FIntPoint BuildingSize) const;
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	bool CanBuildAt(const FIntPoint& GridLocation) const;	// 좌표에 건물을 건설할 수 있는지 검사
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	void PlaceBuildingAt(const FIntPoint& GridLocation, AActor* Building);	// 좌표에 건물을 건설하고 데이터 업데이트
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	void RemoveBuildingAt(const FIntPoint& GridLocation);
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Grid|Settings")
	float CellSize = 200.0f;
	
	UPROPERTY(VisibleAnywhere, Category="Grid|Settings")
	FIntPoint GridDimensions;
	
	UPROPERTY(VisibleAnywhere, Category="Grid|Settings")
	FVector GridOrigin; // 그리드 기준점 이동 변수 / 현재 테스트 레벨 사용중이라 미사용
	
	TArray<FTWGridCellData> GridData;	
	
	bool IsValidGridLocation(const FIntPoint& GridLocation) const;	// 그리드 좌표 유효 범위 확인
};
