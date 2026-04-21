#pragma once

#include "CoreMinimal.h"
#include "MassCommonTypes.h"
#include "MassEntityQuery.h"
#include "Data/TWBuildingTypes.h"
#include "Data/TWUnitStatus.h"
#include "Data/TWUnitTableRowBase.h"
#include "Subsystems/WorldSubsystem.h"
#include "TWUnitSubsystem.generated.h"

class UDataTable;
class UTWPlayerUnitContainer;
class ATWPlayerController;
class ATWUnit;
class ATWBaseBuilding;
struct FMassEntityHandle;
struct FMassReplicationEntityInfo;

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTWUnitSelectionVisualStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SelectionVisual")
	float SelectionCircleRadius = 54.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SelectionVisual")
	float CircleThickness = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SelectionVisual")
	float LocationInterpSpeed = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SelectionVisual")
	float LocationSnapDistance = 78.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SelectionVisual")
	float LocationMaxInterpSpeedMultiplier = 5.0f;
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTWAttackableBuildingCandidate
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ATWBaseBuilding> Building = nullptr;

	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY()
	float DistSquared = TNumericLimits<float>::Max();

	bool IsValid() const
	{
		return Building.IsValid();
	}
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

#ifdef WITH_SERVER_CODE
public:
	void AddPlayer(int32 PlayerSlot);

	bool FindNearestOwnedEntity(
		const FVector& Location,
		FMassEntityHandle& OutEntityHandle,
		int32 OwnerPlayerSlot,
		float MaxDistance = 100.0f
	);

	bool FindNearestAnyEntity(
		const FVector& Location,
		FMassEntityHandle& OutEntityHandle,
		float MaxDistance = 100.0f
	);

	bool FindNearestEnemyEntity(
		const FVector& Location,
		FMassEntityHandle& OutEntityHandle,
		int32 OwnerPlayerSlot,
		float MaxDistance = 100.0f
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

	bool SpawnUnitById(
		const FVector& Location,
		FName UnitId,
		ATWPlayerController* PlayerController
	);

	bool SpawnHeroUnitById(
		const FVector& Location,
		FName HeroUnitId,
		ATWPlayerController* PlayerController
	);

	bool IsHeroUnitId(FName UnitId) const;

	void OnUnitKilled(FMassEntityHandle& Unit);

	TMap<EResourceType, int32> GetUpkeep(int32 PlayerSlot);
	int32 GetCurrentPopulation(int32 PlayerSlot) const;

	bool FindNearestEnemyBuilding(
		const FVector& Location,
		ATWBaseBuilding*& OutBuilding,
		int32 OwnerPlayerSlot,
		float MaxDistance = 1200.0f
	);

	bool FindNearestEnemyBuildingCandidate(
		const FVector& Location,
		int32 OwnerPlayerSlot,
		FTWAttackableBuildingCandidate& OutCandidate,
		float MaxDistance = 1200.0f
	);

	bool GetFriendlyEntitiesInRadius(
		const FVector& Center,
		float Radius,
		int32 OwnerPlayerSlot,
		TArray<FMassEntityHandle>& OutEntityHandles,
		bool bIncludeHeroes
	);

	bool TryGetUnitWorldLocation(
		const FMassEntityHandle& Entity,
		FVector& OutLocation
	) const;

	void ApplyTemporaryMultiplierBuffToFriendlyUnits(
		const FVector& Center,
		float Radius,
		int32 OwnerPlayerSlot,
		float Multiplier,
		float Duration,
		const TArray<ETWStatusType>& TargetStatusTypes,
		bool bIncludeHeroes
	);
	bool GetEnemyEntitiesInRadius(
	const FVector& Center,
	float Radius,
	int32 RequestOwnerPlayerSlot,
	TArray<FMassEntityHandle>& OutEntityHandles
);

	bool FindNearestEnemyEntityAroundLocation(
		const FVector& Center,
		float SearchRadius,
		int32 RequestOwnerPlayerSlot,
		FMassEntityHandle& OutEntityHandle
	);

	bool TryApplyDamageToEntity(
		const FMassEntityHandle& Entity,
		float DamageAmount
	);

private:
	void AddUnit(int32 PlayerSlot, FMassEntityHandle& Unit);
	void RemoveUnit(int32 PlayerSlot, int32 Idx);
#endif

#ifdef WITH_CLIENT_CODE
public:
	bool TryGetUnitVisualLocation(const FMassNetworkID& UnitNetID, FVector& OutLocation) const;
	bool TryGetUnitHPBarWorldLocation(const FMassNetworkID& UnitNetID, FVector& OutLocation) const;
	bool TryGetUnitCurrentHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutCurrentHP) const;
	bool TryGetUnitMaxHP(const FMassNetworkID& UnitNetID, int32 PlayerSlot, float& OutMaxHP) const;
	bool TryGetUnitID(const FMassNetworkID& UnitNetID, FName& OutUnitID) const;
	bool TryGetUnitOwnerPlayerSlot(const FMassNetworkID& UnitNetID, int32& OutOwnerPlayerSlot) const;

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
	const FTWUnitTableRowBase* GetUnitTableRowBase(FName UnitID) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> UnitTable = nullptr;

private:
	UPROPERTY()
	TMap<int32, TObjectPtr<UTWPlayerUnitContainer>> UnitContainers;

	FMassEntityQuery FindNearestEntityQuery;

	// USTRUCT 포인터 캐시 대신 값 복사 캐시
	TMap<FName, FTWUnitTableRowBase> CachedUnitTableRows;
};