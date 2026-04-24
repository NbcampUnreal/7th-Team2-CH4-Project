#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "TWHeroTableRowBase.generated.h"

class AActor;
class UParticleSystem;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTWHeroTableRowBase : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	FName HeroSkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float SkillCooldown = 5.0f;

	// 버프형 스킬 반경 / 투사체형 스킬 사거리 / 기본 스킬 기준 범위
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float SkillRange = 500.0f;

	// 실제 데미지 적용 반경. 0 이하이면 TargetDecalRadius, 그것도 0 이하이면 SkillRange 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float DamageRadius = 0.0f;

	// 단일 타겟/투사체형 스킬에서 타겟을 흡착 탐색할 반경
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float TargetAcquireRadius = 0.0f;

	// 투사체형 스킬일 때 사용할 액터 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	TSubclassOf<AActor> ProjectileClass;

	// 버프형 스킬에서 사용할 스탯 배수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float StatMultiplier = 1.0f;

	// 버프 지속시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float BuffDuration = 0.0f;

	// 공격형 스킬 최종 데미지 배수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float SkillMultiplier = 1.0f;

	// 조준 중 보여줄 데칼 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Targeting")
	TObjectPtr<UMaterialInterface> TargetDecalMaterial = nullptr;

	// 조준 데칼 반경. 0 이하이면 SkillRange를 fallback으로 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Targeting")
	float TargetDecalRadius = 0.0f;

	// 스킬 발동 순간 보여줄 임팩트 데칼 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Impact")
	TObjectPtr<UMaterialInterface> ImpactDecalMaterial = nullptr;

	// 발동 데칼 표시 반경. 0 이하이면 DamageRadius, 그것도 0 이하이면 TargetDecalRadius 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Impact")
	float ImpactDecalRadius = 0.0f;

	// 발동 데칼 유지 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Impact")
	float ImpactDecalLifeTime = 0.75f;

	// 유닛 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Sound")
	FGameplayTag SkillSoundTag;
	
	// 발동 시 사용할 Niagara FX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Impact")
	TObjectPtr<UNiagaraSystem> ImpactNiagaraFX = nullptr;

	// 발동 시 사용할 Cascade FX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Impact")
	TObjectPtr<UParticleSystem> ImpactParticleFX = nullptr;

	// 투사체 시작 위치 보정값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Projectile")
	float ProjectileSpawnZOffset = 50.0f;

	// 디버그 원/구 표시 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill|Debug")
	bool bUseSkillDebugDraw = false;
};
