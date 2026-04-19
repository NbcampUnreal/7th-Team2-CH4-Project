#pragma once

#include "CoreMinimal.h"
#include "Mass/TWUnit.h"
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
	
	virtual uint8 IsIndicatorRequired() const {return false;}
	virtual uint8 IsRangeRequired() const {return false;}
	
	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Skill")
	FVector CurrentTargetLocation;
	
	uint8 GetSkillReady() const {return bIsSkillReady ? true : false;};
	
protected:
	uint8 TryStartCooldown();
	
	void HandleSkillReady();
	
	UPROPERTY(EditDefaultsOnly, Category = "Hero|Skill")
	TObjectPtr<UDataTable> SkillDataTable;

	UPROPERTY(EditDefaultsOnly, Category = "Hero|Skill")
	FName SkillOwnHero;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Hero|Skill")
	uint8 bIsSkillReady = true;
	
	FTimerHandle SkillCooldownTimerHandle;

	UFUNCTION(BlueprintImplementableEvent, Category = "Hero|Skill")
	void OnSkillCooldownStarted(float Duration);
	UFUNCTION(BlueprintImplementableEvent, Category = "Hero|Skill")
	void OnSkillReady();
	
public:
	// UI에서 쿨타임 표시할때 사용
	UFUNCTION(BlueprintCallable, Category = "Hero|UI")
	float GetRemainingCooldownTime() const;
	
	UFUNCTION(BlueprintCallable, Category = "Hero|UI")
	float GetCooldownPercentage() const;
};
