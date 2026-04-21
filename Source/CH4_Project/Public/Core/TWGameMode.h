#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TWGameMode.generated.h"

class APlayerController;
class AController;
class ATWPlayerState;
class ATWBaseBuilding;
class ATWNexusBuilding;

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

	bool SpawnSelectedHeroUnitForPlayer(ATWPlayerState* PS, ATWNexusBuilding* Nexus);

	void BindPlacedBuildingsForPlayer(ATWPlayerState* InPlayerState);
	ATWPlayerState* FindPlayerStateBySlot(int32 InPlayerSlot) const;

	FVector CalculateHeroSpawnLocationFromNexus(const ATWNexusBuilding* Nexus);
	FRotator CalculateHeroSpawnRotationFromNexus(const ATWNexusBuilding* Nexus);

private:
	// 맵의 시작 넥서스 전체 중심점을 계산
	FVector CalculateAllStartNexusCenter() const;

	// 현재 넥서스 기준으로 "맵 안쪽 방향" 벡터 계산
	FVector CalculateMapInwardDirectionFromNexus(const ATWNexusBuilding* Nexus) const;

protected:
	// 넥서스 경계 바깥으로 추가로 얼마나 떨어뜨릴지
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0.0"))
	float StartHeroSpawnDistance = 650.0f;

	// 넥서스 크기 + SpawnDistance 외에 추가 여유 거리
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0.0"))
	float StartHeroSpawnExtraPadding = 180.0f;

	// 좌우로 너무 일직선만 되지 않게 살짝 퍼뜨리는 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0.0"))
	float StartHeroLateralRandomOffset = 120.0f;

	// 최종 회전 보정
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero")
	float StartHeroYawOffset = 0.0f;

	// 혹시 바닥 보정용 trace를 별도로 쓰는 경우 대비 유지
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0.0"))
	float StartHeroGroundTraceHalfHeight = 500.0f;

	// NavMesh 투영 범위
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0.0"))
	float StartHeroNavProjectExtentXY = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0.0"))
	float StartHeroNavProjectExtentZ = 300.0f;

	// PostLogin 이후 약간 늦춰 초기화
	UPROPERTY(EditDefaultsOnly, Category = "Start|Init", meta = (ClampMin = "0.0"))
	float PostLoginInitializeDelay = 0.15f;

	// 영웅 스폰 실패 시 재시도 딜레이
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0.0"))
	float InitialHeroSpawnRetryDelay = 0.3f;

	// 영웅 스폰 최대 재시도 횟수
	UPROPERTY(EditDefaultsOnly, Category = "Start|Hero", meta = (ClampMin = "0"))
	int32 InitialHeroSpawnMaxRetryCount = 5;
};