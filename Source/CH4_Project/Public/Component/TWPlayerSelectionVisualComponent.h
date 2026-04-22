#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MassCommonTypes.h"
#include "TWPlayerSelectionVisualComponent.generated.h"

class ATWPlayerController;
class ATWBaseBuilding;
class UStaticMesh;
class UMaterialInterface;
class ATWSelectionVisualActor;

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTWSelectedVisualData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	bool bValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	bool bIsBuilding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	int32 OwnerPlayerSlot = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	FVector SelectionWorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	FVector HPBarWorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	float CurrentHP = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	float MaxHP = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	float HealthPercent = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	float SelectionRadius = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	FVector2D BuildingHalfExtentXY = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="SelectionVisual")
	float BuildingZOffset = 0.f;

	void Reset()
	{
		bValid = false;
		bIsBuilding = false;
		OwnerPlayerSlot = INDEX_NONE;
		SelectionWorldLocation = FVector::ZeroVector;
		HPBarWorldLocation = FVector::ZeroVector;
		CurrentHP = 0.f;
		MaxHP = 0.f;
		HealthPercent = 0.f;
		SelectionRadius = 0.f;
		BuildingHalfExtentXY = FVector2D::ZeroVector;
		BuildingZOffset = 0.f;
	}
};

USTRUCT()
struct CH4_PROJECT_API FTWUnitRingVisualData
{
	GENERATED_BODY()

	UPROPERTY()
	FMassNetworkID UnitNetId;

	UPROPERTY()
	FVector RingWorldLocation = FVector::ZeroVector;

	UPROPERTY()
	float RingRadius = 54.f;
};

USTRUCT()
struct CH4_PROJECT_API FTWBuildingSelectionVisualData
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ATWBaseBuilding> Building;

	UPROPERTY()
	int32 OwnerPlayerSlot = INDEX_NONE;

	UPROPERTY()
	FVector SelectionWorldLocation = FVector::ZeroVector;

	UPROPERTY()
	FVector2D BuildingHalfExtentXY = FVector2D::ZeroVector;

	UPROPERTY()
	float BuildingZOffset = 0.f;
};

USTRUCT()
struct CH4_PROJECT_API FTWHPBarVisualData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bValid = false;

	UPROPERTY()
	bool bIsBuilding = false;

	UPROPERTY()
	int32 OwnerPlayerSlot = INDEX_NONE;

	UPROPERTY()
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY()
	float CurrentHP = 0.f;

	UPROPERTY()
	float MaxHP = 0.f;

	UPROPERTY()
	float HealthPercent = 0.f;

	UPROPERTY()
	FVector WorldScale = FVector(1.f, 0.2f, 0.08f);

	void Reset()
	{
		bValid = false;
		bIsBuilding = false;
		OwnerPlayerSlot = INDEX_NONE;
		WorldLocation = FVector::ZeroVector;
		CurrentHP = 0.f;
		MaxHP = 0.f;
		HealthPercent = 0.f;
		WorldScale = FVector(1.f, 0.2f, 0.08f);
	}
};

USTRUCT()
struct CH4_PROJECT_API FTWRecentCombatHPBarData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bValid = false;

	UPROPERTY()
	bool bIsBuilding = false;

	UPROPERTY()
	FMassNetworkID UnitNetId;

	UPROPERTY()
	TWeakObjectPtr<ATWBaseBuilding> Building;

	UPROPERTY()
	int32 OwnerPlayerSlot = INDEX_NONE;

	UPROPERTY()
	float RemainingTime = 0.f;

	UPROPERTY()
	FVector CachedWorldLocation = FVector::ZeroVector;

	UPROPERTY()
	float CachedHealthPercent = 1.f;

	UPROPERTY()
	float CachedCurrentHP = 0.f;

	UPROPERTY()
	float CachedMaxHP = 0.f;

	void Reset()
	{
		bValid = false;
		bIsBuilding = false;
		UnitNetId = FMassNetworkID();
		Building = nullptr;
		OwnerPlayerSlot = INDEX_NONE;
		RemainingTime = 0.f;
		CachedWorldLocation = FVector::ZeroVector;
		CachedHealthPercent = 1.f;
		CachedCurrentHP = 0.f;
		CachedMaxHP = 0.f;
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CH4_PROJECT_API UTWPlayerSelectionVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTWPlayerSelectionVisualComponent();

	void InitializeVisuals(ATWPlayerController* InOwnerController);

	void RefreshSelectionVisuals(
		const TArray<FMassNetworkID>& InSelectedUnitNetIds,
		ATWBaseBuilding* InSelectedBuilding,
		int32 InSelectedOwnerPlayerSlot
	);

	void ClearSelectionVisuals();
	void TickVisuals(float DeltaSeconds);

	void UpdateSelectionVisualData(
		const TArray<FMassNetworkID>& InSelectedUnitNetIds,
		ATWBaseBuilding* InSelectedBuilding,
		int32 InSelectedOwnerPlayerSlot
	);

	void NotifyRecentCombatUnitDamaged(
		const FMassNetworkID& InUnitNetId,
		int32 InOwnerPlayerSlot,
		float InVisibleTime = 1.25f
	);

	void NotifyRecentCombatBuildingDamaged(
		ATWBaseBuilding* InBuilding,
		float InVisibleTime = 1.25f
	);

	const FTWSelectedVisualData& GetPrimarySelectedVisualData() const { return PrimarySelectedVisualData; }
	const TArray<FTWUnitRingVisualData>& GetSelectedUnitRingVisuals() const { return SelectedUnitRingVisuals; }
	const TArray<FTWBuildingSelectionVisualData>& GetSelectedBuildingVisuals() const { return SelectedBuildingVisuals; }
	const TArray<FTWHPBarVisualData>& GetSelectedHPBarVisuals() const { return SelectedHPBarVisuals; }

private:
	void ClearCachedVisualData();

	bool BuildSelectedUnitVisualData(
		const TArray<FMassNetworkID>& InSelectedUnitNetIds,
		int32 InSelectedOwnerPlayerSlot
	);
	bool BuildSelectedUnitVisualDataFromEntityHandles(
		int32 InSelectedOwnerPlayerSlot
	);

	bool BuildSelectedBuildingVisualData(ATWBaseBuilding* InSelectedBuilding);
	void TickRecentCombatHPBars(float DeltaSeconds);
	void BuildRecentCombatHPBarVisualData();
	void RemoveExpiredRecentCombatHPBars();

private:
	void InitializeRenderBackend();
	void SpawnRenderActorIfNeeded();
	void DestroyRenderActorIfNeeded();
	void HideAllRenderers();
	void SyncRenderBackendFromCachedData();

private:
	UPROPERTY(Transient)
	TObjectPtr<ATWPlayerController> OwnerController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ATWSelectionVisualActor> SelectionVisualActor = nullptr;

	UPROPERTY(Transient)
	FTWSelectedVisualData PrimarySelectedVisualData;

	UPROPERTY(Transient)
	TArray<FTWUnitRingVisualData> SelectedUnitRingVisuals;

	UPROPERTY(Transient)
	TArray<FTWHPBarVisualData> RecentCombatHPBarVisuals;

	UPROPERTY(Transient)
	TArray<FTWRecentCombatHPBarData> RecentCombatHPBars;

	UPROPERTY(Transient)
	TArray<FTWBuildingSelectionVisualData> SelectedBuildingVisuals;

	UPROPERTY(Transient)
	TArray<FTWHPBarVisualData> SelectedHPBarVisuals;

	UPROPERTY(Transient)
	TArray<FMassNetworkID> LastSelectedUnitNetIds;

	UPROPERTY(Transient)
	TWeakObjectPtr<ATWBaseBuilding> LastSelectedBuilding;

	UPROPERTY(Transient)
	int32 LastSelectedOwnerPlayerSlot = INDEX_NONE;

	UPROPERTY(Transient)
	bool bVisualsInitialized = false;

	UPROPERTY(Transient)
	bool bRenderActorSpawnRequested = false;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units", meta=(ClampMin="0.0"))
	float DefaultUnitSelectionCircleRadius = 54.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Buildings", meta=(ClampMin="0.0"))
	float DefaultBuildingSelectionPadding = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Buildings", meta=(ClampMin="0.0"))
	float DefaultBuildingSelectionZOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Buildings", meta=(ClampMin="0.0"))
	float BuildingGroundLift = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|HPBar", meta=(ClampMin="0.0"))
	float DefaultFallbackUnitHPBarHeight = 120.f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units")
	TObjectPtr<UStaticMesh> UnitRingMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units")
	TObjectPtr<UMaterialInterface> UnitRingMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units", meta=(ClampMin="1.0"))
	float UnitRingMeshBaseDiameter = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units", meta=(ClampMin="0.0"))
	float UnitRingZOffset = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units")
	bool bRotateUnitRingToGroundPlane = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units")
	FRotator UnitRingRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Units", meta=(ClampMin="0.01"))
	float UnitRingScaleMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Buildings")
	TObjectPtr<UStaticMesh> BuildingSelectionBoxMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Buildings")
	TObjectPtr<UMaterialInterface> BuildingSelectionBoxMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|Buildings", meta=(ClampMin="0.01"))
	float BuildingSelectionBoxThickness = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|HPBar")
	TObjectPtr<UStaticMesh> HPBarMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|HPBar")
	TObjectPtr<UMaterialInterface> HPBarMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|HPBar")
	FVector HPBarWorldScale = FVector(1.0f, 0.2f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|CombatFeedback", meta=(ClampMin="0.0"))
	float RecentCombatHPBarVisibleTime = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SelectionVisual|CombatFeedback", meta=(ClampMin="1"))
	int32 MaxRecentCombatHPBarCount = 24;
};
