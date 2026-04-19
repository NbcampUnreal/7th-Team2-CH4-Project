#include "HeroUnit/TWHeroUnitBuff.h"

#include "Data/TWHeroTableRowBase.h"

struct FTWHeroTableRowBase;

ATWHeroUnitBuff::ATWHeroUnitBuff()
{
}

void ATWHeroUnitBuff::UseSkill()
{
	UE_LOG(LogTemp, Warning, TEXT("UseSkill() 호출됨"));
	
	if (TryStartCooldown())
	{
		UE_LOG(LogTemp, Log, TEXT("본인 버프 스킬 시전!"));
		static const FString ContextString(TEXT("BuffHeroContext"));
		FTWHeroTableRowBase* SkillData = SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString);

		if (SkillData)
		{
			ApplySelfBuff();
			
			FTimerHandle BuffTimerHandle;
			GetWorldTimerManager().SetTimer(BuffTimerHandle, [this]()
			{
				EndSelfBuff();
				UE_LOG(LogTemp, Log, TEXT("버프 종료"));
			},  
			SkillData->BuffDuration, false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (쿨타임 or 데이터 찾을 수 없음)"));
	}
}

void ATWHeroUnitBuff::ApplySelfBuff()
{
	// 스텟 상승 로직
}

void ATWHeroUnitBuff::EndSelfBuff()
{
	// 스텟 복구 로직
}
