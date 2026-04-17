#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Component/TWPlayerSelectionVisualComponent.h"
#include "TWSelectionVisualActor.generated.h"

class USceneComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class CH4_PROJECT_API ATWSelectionVisualActor : public AActor
{
	GENERATED_BODY()

public:
	ATWSelectionVisualActor();

	void ConfigureRenderAssets(
		UStaticMesh* InUnitRingMesh,
		UMaterialInterface* InUnitRingMaterial,
		float InUnitRingMeshBaseDiameter,
		float InUnitRingZOffset,
		bool bInRotateUnitRingToGroundPlane,
		const FRotator& InUnitRingRotationOffset,
		float InUnitRingScaleMultiplier,
		UStaticMesh* InBuildingSelectionBoxMesh,
		UMaterialInterface* InBuildingSelectionBoxMaterial,
		float InBuildingSelectionBoxThickness,
		UStaticMesh* InHPBarMesh,
		UMaterialInterface* InHPBarMaterial,
		const FVector& InHPBarWorldScale
	);

	void SetVisualData(
		const FTWSelectedVisualData& InPrimarySelectedVisualData,
		const TArray<FTWUnitRingVisualData>& InSelectedUnitRingVisuals,
		const TArray<FTWBuildingSelectionVisualData>& InSelectedBuildingVisuals,
		const TArray<FTWHPBarVisualData>& InSelectedHPBarVisuals
	);

	void ClearVisuals();
	void SyncVisuals();

protected:
	virtual void BeginPlay() override;

private:
	void SyncUnitRingISM();
	void SyncBuildingSelectionBoxISM();
	void SyncHPBarISM();

	int32 ResolveLocalPlayerSlot() const;
	FLinearColor ResolveSlotBaseColor(int32 InOwnerPlayerSlot) const;
	FLinearColor ApplyRelationTint(const FLinearColor& InBaseColor, int32 InOwnerPlayerSlot) const;
	FLinearColor ResolveFinalOwnerColor(int32 InOwnerPlayerSlot) const;

private:
	UPROPERTY(VisibleAnywhere, Category="SelectionVisual")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category="SelectionVisual")
	TObjectPtr<UInstancedStaticMeshComponent> UnitRingISM = nullptr;

	UPROPERTY(VisibleAnywhere, Category="SelectionVisual")
	TObjectPtr<UInstancedStaticMeshComponent> BuildingSelectionBoxISM = nullptr;

	UPROPERTY(VisibleAnywhere, Category="SelectionVisual")
	TObjectPtr<UInstancedStaticMeshComponent> HPBarISM = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> UnitRingMID = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BuildingSelectionBoxMID = nullptr;

private:
	UPROPERTY(Transient)
	FTWSelectedVisualData PrimarySelectedVisualData;

	UPROPERTY(Transient)
	TArray<FTWUnitRingVisualData> SelectedUnitRingVisuals;

	UPROPERTY(Transient)
	TArray<FTWBuildingSelectionVisualData> SelectedBuildingVisuals;

	UPROPERTY(Transient)
	TArray<FTWHPBarVisualData> SelectedHPBarVisuals;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> UnitRingMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> UnitRingMaterial = nullptr;

	UPROPERTY(Transient)
	float UnitRingMeshBaseDiameter = 100.f;

	UPROPERTY(Transient)
	float UnitRingZOffset = 3.f;

	UPROPERTY(Transient)
	bool bRotateUnitRingToGroundPlane = true;

	UPROPERTY(Transient)
	FRotator UnitRingRotationOffset = FRotator(90.f, 0.f, 0.f);

	UPROPERTY(Transient)
	float UnitRingScaleMultiplier = 1.f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> BuildingSelectionBoxMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> BuildingSelectionBoxMaterial = nullptr;

	UPROPERTY(Transient)
	float BuildingSelectionBoxThickness = 4.f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> HPBarMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> HPBarMaterial = nullptr;

	UPROPERTY(Transient)
	FVector HPBarWorldScale = FVector(1.f, 0.2f, 0.08f);

protected:
	UPROPERTY(EditAnywhere, Category="SelectionVisual|Color")
	TMap<int32, FLinearColor> PlayerSlotColorMap;

	UPROPERTY(EditAnywhere, Category="SelectionVisual|Color")
	FLinearColor DefaultSlotColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category="SelectionVisual|Color")
	FLinearColor NeutralColor = FLinearColor(0.65f, 0.65f, 0.65f, 1.f);

	UPROPERTY(EditAnywhere, Category="SelectionVisual|Color")
	float EnemyBrightnessMultiplier = 1.20f;

	UPROPERTY(EditAnywhere, Category="SelectionVisual|Color")
	float NeutralDesaturationAlpha = 0.65f;
};