#include "HeroUnit/TWHeroUnitBase.h"

#include "Data/TWHeroTableRowBase.h"

ATWHeroUnitBase::ATWHeroUnitBase()
{
	bIsSkillReady = 1;
}

uint8 ATWHeroUnitBase::TryStartCooldown()
{
	if (!bIsSkillReady || !SkillDataTable || SkillOwnHero.IsNone()) return false;
	
	static const FString ContextString(TEXT("SkillCooldownContext"));
	FTWHeroTableRowBase* Row = SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString);
    
	if (Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 쿨타임 시작"));
		bIsSkillReady = false;
		
		GetWorldTimerManager().SetTimer(
			SkillCooldownTimerHandle, 
			this, 
			&ATWHeroUnitBase::HandleSkillReady, 
			Row->SkillCooldown, 
			false
		);
		
		OnSkillCooldownStarted(Row->SkillCooldown);
		return true;
	}

	return false;
}

void ATWHeroUnitBase::HandleSkillReady()
{
	UE_LOG(LogTemp, Warning, TEXT("스킬 준비됨!!!"));
	bIsSkillReady = true;
	OnSkillReady();
}

// 남은 쿨타임 가져오는 함수
float ATWHeroUnitBase::GetRemainingCooldownTime() const
{
	if (bIsSkillReady) return 0.f;
	
	return GetWorldTimerManager().GetTimerRemaining(SkillCooldownTimerHandle);
}

// 남은 쿨타임 퍼센테이지 가져오는 함수
float ATWHeroUnitBase::GetCooldownPercentage() const
{
	if (bIsSkillReady) return 1.f;
	
	float RemainingCooldownTime = GetRemainingCooldownTime();
	float TotalCooldownTime = GetWorldTimerManager().GetTimerRemaining(SkillCooldownTimerHandle);
	
	if (TotalCooldownTime<0.f) return 1.f;
	
	return 1.f - (RemainingCooldownTime - TotalCooldownTime);
}
