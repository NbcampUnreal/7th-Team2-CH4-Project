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
	Blocking                UMETA(DisplayName="Blocking"),
	Nexus                   UMETA(DisplayName="Nexus")
};

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	Wood    UMETA(DisplayName="Wood"),
	Ore     UMETA(DisplayName="Ore"),
	Count	UMETA(DisplayName="Count"),
	None	UMETA(DisplayName="None"),
};


USTRUCT(BlueprintType)
struct FBuildingGridSize
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Building|Grid", meta=(ClampMin="1"))
	FIntPoint BuildingSize = FIntPoint(1, 1);

};