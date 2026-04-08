#pragma once

#include "CoreMinimal.h"
#include "TWBuildingTypes.generated.h"

UENUM(BlueprintType)
enum class EBuildingCategory : uint8
{
	None                UMETA(DisplayName="None"),
	ResourceProduction  UMETA(DisplayName="ResourceProduction"),
	TroopProduction     UMETA(DisplayName="TroopProduction"),
	PopulationProduction     UMETA(DisplayName="PopulationProduction"),
	Blocking                UMETA(DisplayName="Blocking")
};

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	None    UMETA(DisplayName="None"),
	Wood    UMETA(DisplayName="Wood"),
	Ore     UMETA(DisplayName="Ore")
};

USTRUCT(BlueprintType)
struct FBuildingResourceCost
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Cost", meta=(ClampMin="0"))
	int32 Wood = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Cost", meta=(ClampMin="0"))
	int32 Ore = 0;
};

USTRUCT(BlueprintType)
struct FBuildingGridSize
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Grid", meta=(ClampMin="1"))
	FIntPoint BuildingSize = FIntPoint(1, 1);

};