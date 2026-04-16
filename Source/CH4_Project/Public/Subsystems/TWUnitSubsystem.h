#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "MassEntityQuery.h"
#include "Data/TWBuildingTypes.h"
#include "Data/TWUnitStatus.h"
#include "Subsystems/WorldSubsystem.h"
#include "TWUnitSubsystem.generated.h"

class UTWPlayerUnitContainer;
struct FTWUnitTableRowBase;
class ATWPlayerController;
class ATWUnit;
struct FMassEntityHandle;
struct FMassReplicationEntityInfo;
class UDataTable;

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTWUnitSelectionVisualStyle
{
	GENERATED_BODY()

	/** 선택 링 반지름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SelectionVisual")
	float SelectionCircleRadius = 54.f;

	/** 선택 링 두께 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SelectionVisual")
	float CircleThickness = 2.2f;

	/** 선택 표시 위치 보간 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SelectionVisual")
	float LocationInterpSpeed = 35.f;

	/** 너무 멀면 즉시 스냅 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SelectionVisual")
	float LocationSnapDistance = 78.f;

	/** 거리 클수록 보간 가속 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="SelectionVisual")
	float LocationMaxInterpSpeedMultiplier = 5.0f;
};

UCLASS()
class CH4_PROJECT_API UTWUnitSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UTWUnitSubsystem();

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;

#ifdef WITH_SERVER_CODE
public:
	void AddPlayer(int32 PlayerSlot);

	bool FindNearestEntity(
		const FVector& Location,
		FMassEntityHandle& OutEntityHandle,
		int32 OwnerPlayerSlot,
		float MaxDistance = 50.0f
	);

	bool GetEntitiesInRectangle(
		const FVector& StartLocation,
		const FVector& EndLocation,
		TArray<FMassEntityHandle>& OutEntityHandles,
		int32 OwnerPlayerSlot
	);

	void SpawnUnit(
		const FVector& Location,
		const FTWUnitTableRowBase& UnitTableRowBase,
		ATWPlayerController* PlayerController
	);

	TMap<EResourceType, int32> GetUpkeep(int32 PlayerSlot);
	int32 GetCurrentPopulation(int32 PlayerSlot) const;

private:
	void AddUnit(int32 PlayerSlot, FMassEntityHandle& Unit);
	void RemoveUnit(int32 PlayerSlot, int32 Idx);
#endif

public:
#pragma region Status
	FTWUnitStatus GetUnitDefaultStatus(FName UnitID, int32 PlayerSlot);
	FTWUnitStatus GetUnitCurrentStatus(const FMassEntityHandle& Unit, int32 PlayerSlot) const;

#ifdef WITH_CLIENT_CODE
	FTWUnitStatus GetUnitCurrentStatus(const FMassNetworkID& UnitNetID, int32 PlayerSlot) const;
#endif

#ifdef WITH_SERVER_CODE
	void ApplyStatus(FName UnitID, int32 PlayerSlot);
#endif
#pragma endregion

#ifdef WITH_CLIENT_CODE
public:
	/** 선택 링용 월드 위치 */
	bool TryGetUnitVisualLocation(const FMassNetworkID& UnitNetID, FVector& OutLocation) const;

	/** HP 바용 월드 위치 */
	bool TryGetUnitHPBarWorldLocation(const FMassNetworkID& UnitNetID, FVector& OutLocation) const;

	/** 현재 HP */
	bool TryGetUnitCurrentHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutCurrentHP) const;

	/** 최대 HP */
	bool TryGetUnitMaxHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutMaxHP) const;

	/** 유닛 타입 ID */
	bool TryGetUnitID(const FMassNetworkID& UnitNetID, FName& OutUnitID) const;

	/** 선택 비주얼 스타일 */
	bool TryGetUnitSelectionVisualStyle(
		const FMassNetworkID& UnitNetID,
		FTWUnitSelectionVisualStyle& OutStyle
	) const;

private:
	bool TryGetReplicationEntityInfo(
		const FMassNetworkID& UnitNetID,
		const FMassReplicationEntityInfo*& OutEntityInfo
	) const;

	bool TryGetUnitVisualActor(
		const FMassNetworkID& UnitNetID,
		const ATWUnit*& OutVisualActor
	) const;

	bool TryGetUnitLocationInternal(
		const FMassNetworkID& UnitNetID,
		FVector& OutLocation
	) const;

	bool TryProjectWorldPointToGround(
		const FVector& InWorldPoint,
		FVector& OutGroundPoint,
		const AActor* ActorToIgnore = nullptr
	) const;
#endif

public:
	FTWUnitTableRowBase* GetUnitTableRowBase(FName UnitID) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> UnitTable;

private:
	UPROPERTY()
	TMap<int32, UTWPlayerUnitContainer*> UnitContainers;

	FMassEntityQuery FindNearestEntityQuery;
	TMap<FName, FTWUnitTableRowBase*> CachedUnitTableRows;
};