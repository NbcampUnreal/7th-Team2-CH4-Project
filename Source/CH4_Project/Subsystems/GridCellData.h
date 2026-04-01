#pragma once

#include "CoreMinimal.h"
#include "GridCellData.generated.h"

UENUM(BlueprintType)
enum class ECellVisibilityState : uint8
{
	Hidden		UMETA(DisplayName = "Hidden"),
	Explored	UMETA(DisplayName = "Explored"),
	Visible		UMETA(DisplayName = "Visible")
};

USTRUCT(BlueprintType)
struct FGridCellData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Flags")
	uint8 bIsBuildable : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Flags")
	uint8 bIsDestroyed : 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Flags")
	uint8 bHasTrap : 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Construction")
	TObjectPtr<AActor> OccupyingBuilding;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Construction")
	ECellVisibilityState VisibilityState;
	
	FGridCellData()
		: bIsBuildable(true)
		, bIsDestroyed(false)
		, bHasTrap(false)
		, OccupyingBuilding(nullptr)
		, VisibilityState(ECellVisibilityState::Hidden)
	{}
};
