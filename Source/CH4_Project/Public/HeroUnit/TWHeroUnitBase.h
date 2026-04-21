#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Mass/TWUnit.h"
#include "Data/TWUnitStatus.h"
#include "TWHeroUnitBase.generated.h"

UCLASS(Abstract)
class CH4_PROJECT_API ATWHeroUnitBase : public ATWUnit
{
	GENERATED_BODY()

public:
	ATWHeroUnitBase();

	virtual void UseSkill() PURE_VIRTUAL(ATWHeroUnitBase::UseSkill, );

	virtual void SetIndicatorVisible(bool Visible) {}
	virtual void SetRangeVisible(bool Visible) {}
	virtual void UpdateIndicator(FVector MouseLocation) {}

	virtual uint8 IsIndicatorRequired() const { return false; }
	virtual uint8 IsRangeRequired() const { return false; }

	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Skill")
	FVector CurrentTargetLocation = FVector::ZeroVector;

	uint8 GetSkillReady() const { return bIsSkillReady ? true : false; }

	UFUNCTION(BlueprintCallable, Category = "Hero|UI")
	float GetRemainingCooldownTime() const;

	UFUNCTION(BlueprintCallable, Category = "Hero|UI")
	float GetCooldownPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Hero|Skill")
	bool CommitSkillCooldown();

	UFUNCTION(BlueprintCallable, Category = "Hero|Combat")
	float GetCurrentHeroHP() const { return CurrentHeroHP; }

	UFUNCTION(BlueprintCallable, Category = "Hero|Combat")
	float GetMaxHeroHP() const { return RuntimeHeroStatus.GetStatus(ETWStatusType::Health); }

	UFUNCTION(BlueprintCallable, Category = "Hero|Combat")
	float GetHeroDamageStat() const;

	UFUNCTION(BlueprintCallable, Category = "Hero|Combat")
	void ReceiveHeroDamage(float InDamageAmount, AActor* DamageCauser);

	const FTWUnitStatus& GetRuntimeHeroStatus() const { return RuntimeHeroStatus; }

protected:
	virtual void BeginPlay() override;

	uint8 TryStartCooldown();
	void HandleSkillReady();

	bool IsSameTeamActor(const AActor* OtherActor) const;
	bool TryApplyDamageToActor(AActor* TargetActor, float DamageAmount);

	void ApplyHeroBuffMultiplier(float InMultiplier);
	void RestoreHeroBuff();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Skill")
	TObjectPtr<UDataTable> SkillDataTable = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Hero|Skill")
	FName SkillOwnHero = NAME_None;

	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Skill")
	uint8 bIsSkillReady = true;

	FTimerHandle SkillCooldownTimerHandle;

	UFUNCTION(BlueprintImplementableEvent, Category = "Hero|Skill")
	void OnSkillCooldownStarted(float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hero|Skill")
	void OnSkillReady();

protected:
	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Combat")
	float CurrentHeroHP = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Combat")
	FTWUnitStatus BaseHeroStatus;

	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Combat")
	FTWUnitStatus RuntimeHeroStatus;

	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Buff")
	FTWUnitStatus CachedPreBuffStatus;

	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Buff")
	bool bHeroBuffApplied = false;
};