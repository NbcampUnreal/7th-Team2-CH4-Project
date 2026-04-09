#pragma once

#include "CoreMinimal.h"
#include "TWGridCellData.generated.h"

UENUM(BlueprintType)
enum class ECellVisibilityState : uint8
{
	Hidden		UMETA(DisplayName = "Hidden"),
	Explored	UMETA(DisplayName = "Explored"),
	Visible		UMETA(DisplayName = "Visible")
};

USTRUCT(BlueprintType)
struct FTWGridCellData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Flags")
	uint8 bIsBuildable : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Flags")
	uint8 bIsDestroyed : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Flags")
	uint8 bHasTrap : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Coordinate")
	float TerrarianZ;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Construction")
	TObjectPtr<AActor> OccupyingBuilding;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Construction")
	ECellVisibilityState VisibilityState;
	
	FTWGridCellData()
		: bIsBuildable(true)
		, bIsDestroyed(false)
		, bHasTrap(false)
		, TerrarianZ(0.0f)
		, OccupyingBuilding(nullptr)
		, VisibilityState(ECellVisibilityState::Hidden)
	{}
};
