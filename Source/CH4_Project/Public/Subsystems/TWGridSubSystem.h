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
	
#pragma endregion 
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	bool CanBuildArea(FIntPoint AnchorLocation, FIntPoint BuildingSize) const;
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	void OccupyArea(FIntPoint AnchorLocation, FIntPoint BuildingSize, AActor* BuildingAnchor);
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Coordinate")
	FVector GetBuildingCenterPosition(FIntPoint AnchorLocation, FIntPoint BuildingSize) const;
	
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	bool CanBuildAt(const FIntPoint& GridLocation) const;	// 좌표에 건물을 건설할 수 있는지 검사
	UFUNCTION(BlueprintCallable, Category = "Grid|Construction")
	void PlaceBuildingAt(const FIntPoint& GridLocation, AActor* Building);	// 좌표에 건물을 건설하고 데이터 업데이트
	
	// UFUNCTION(BlueprintCallable, Category = "Grid|FogOfWar")
	// void UpdateVisionAroundLocation(const FIntPoint& CenterLocation, int32 VisionRadius);	// 건물 주변 반경 시야 Visible로 갱신
	//
	
private:
	UPROPERTY(EditDefaultsOnly, Category="Grid|Settings")
	float CellSize = 100.0f;
	
	UPROPERTY(EditDefaultsOnly, Category="Grid|Settings")
	FIntPoint GridDimensions = FIntPoint(200, 200);
	
	UPROPERTY(EditDefaultsOnly, Category="Grid|Settings")
	FVector GridOrigin = FVector(-3100.0f, -3100.0f, 0.0f); // 그리드 기준점 이동 변수 / 현재 테스트 레벨 사용중이라 미사용
	
	TArray<FTWGridCellData> GridData;	
	
	bool IsValidGridLocation(const FIntPoint& GridLocation) const;	// 그리드 좌표 유효 범위 확인
};
