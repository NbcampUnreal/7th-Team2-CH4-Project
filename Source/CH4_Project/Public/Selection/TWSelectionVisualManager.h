#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MassCommonTypes.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Component/TWPlayerSelectionVisualComponent.h"
#include "TWSelectionVisualManager.generated.h"

class ATWPlayerController;
class ATWBaseBuilding;

UCLASS()
class CH4_PROJECT_API UTWSelectionVisualManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(ATWPlayerController* InOwnerController);

	void ApplyUnitSelection(const TArray<FMassNetworkID>& InSelectedUnits);
	void ApplyBuildingSelection(const TArray<ATWBaseBuilding*>& InSelectedBuildings);
	void ClearSelectionVisuals();
	void SetPrimarySelectedVisualData(const FTWSelectedVisualData& InData);
	void SetSelectedUnitRingVisuals(const TArray<FTWUnitRingVisualData>& InData);
	void Tick(float DeltaSeconds);

protected:
	void RemoveStaleSmoothedUnitLocations();
	void RebuildCachedUnitVisualStyles();

protected:
	UPROPERTY()
	TObjectPtr<ATWPlayerController> OwnerController = nullptr;

	UPROPERTY()
	TArray<FMassNetworkID> SelectedUnitNetIds;

	UPROPERTY()
	TArray<TWeakObjectPtr<ATWBaseBuilding>> SelectedBuildings;

	UPROPERTY()
	TMap<FMassNetworkID, FVector> SmoothedUnitLocations;

	UPROPERTY()
	TMap<FMassNetworkID, FTWUnitSelectionVisualStyle> CachedUnitVisualStyles;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	bool bEnableDebugDraw = true;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	int32 MaxDebugDrawUnitCount = 100;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float DefaultUnitLocationInterpSpeed = 35.f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float DefaultUnitLocationSnapDistance = 60.f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float DefaultUnitLocationMaxInterpSpeedMultiplier = 5.0f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float DefaultUnitSelectionCircleRadius = 48.f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float DefaultUnitCircleThickness = 2.f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float BuildingSelectionLineThickness = 3.f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float BuildingSelectionPadding = 20.f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual")
	float BuildingSelectionZOffset = 4.f;
private:
	UPROPERTY(Transient)
	FTWSelectedVisualData PrimarySelectedVisualData;

	UPROPERTY(Transient)
	TArray<FTWUnitRingVisualData> SelectedUnitRingVisuals;
};