#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TWGameMode.generated.h"

class APlayerController;
class AController;
class ATWPlayerState;
class ATWBaseBuilding;
class ATWNexusBuilding;
class ATWPlayerController;

UCLASS()
class CH4_PROJECT_API ATWGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	void TryBindBuilding(ATWBaseBuilding* InBuilding);
	void HandlePlayerDefeat(int32 DefeatedPlayerSlot);

	void RequestRespawnHero(ATWPlayerState* PS, FName HeroUnitId, float DelaySeconds = 5.0f);
	void RespawnHero(ATWPlayerState* PS, FName HeroUnitId);

protected:
	void RequestInitializeJoinedPlayer(AController* NewController, float DelaySeconds = 0.0f);
	void InitializeJoinedPlayer(AController* NewController);

	bool HasAlreadyInitializedPlayer(const ATWPlayerState* PS) const;
	bool HasSpawnedInitialHeroForPlayer(const ATWPlayerState* PS) const;

	void ScheduleInitialHeroSpawnRetry(ATWPlayerState* PS, int32 RemainingRetryCount);
	void RetryInitialHeroSpawn(ATWPlayerState* PS, int32 RemainingRetryCount);

	int32 AllocateNextPlayerSlot() const;

	ATWNexusBuilding* AssignRandomStartNexus(ATWPlayerState* PS);
	void AssignNexusOwnership(ATWNexusBuilding* Nexus, ATWPlayerState* PS);

	void EnsurePlayerSeedState(ATWPlayerState* PS);
	ATWPlayerController* FindOwningPlayerController(const ATWPlayerState* PS) const;

	FVector CalculateAllStartNexusCenter() const;
	FVector CalculateMapInwardDirectionFromNexus(const ATWNexusBuilding* Nexus) const;
	FVector CalculateHeroSpawnLocationFromNexus(const ATWNexusBuilding* Nexus);
	FRotator CalculateHeroSpawnRotationFromNexus(const ATWNexusBuilding* Nexus);

	bool SpawnSelectedHeroUnitForPlayer(ATWPlayerState* PS, ATWNexusBuilding* Nexus);

	ATWPlayerState* FindPlayerStateBySlot(int32 InPlayerSlot) const;
	void BindPlacedBuildingsForPlayer(ATWPlayerState* InPlayerState);

	int32 GetAssignedStartPlayerCount() const;
	bool AreAllRequiredPlayersAssignedStartNexus() const;
	void CleanupUnusedStartNexusBuildings();
	void TryFinalizeStartSetup();
	void MovePlayerCameraToAssignedStart(ATWPlayerController* PC, ATWNexusBuilding* Nexus);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Init")
	float PostLoginInitializeDelay = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	int32 InitialHeroSpawnMaxRetryCount = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float InitialHeroSpawnRetryDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float StartHeroSpawnDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float StartHeroSpawnExtraPadding = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float StartHeroLateralRandomOffset = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float StartHeroNavProjectExtentXY = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float StartHeroNavProjectExtentZ = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float StartHeroGroundTraceHalfHeight = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Hero")
	float StartHeroYawOffset = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Start")
	int32 RequiredStartPlayerCount = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|Start")
	bool bHasCleanedUpUnusedStartNexus = false;

};
