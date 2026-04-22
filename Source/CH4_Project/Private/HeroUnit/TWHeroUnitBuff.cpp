#include "HeroUnit/TWHeroUnitBuff.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Data/TWHeroTableRowBase.h"
#include "EngineUtils.h"
#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Log/TWLogCategory.h"

struct FTWHeroTableRowBase;

ATWHeroUnitBuff::ATWHeroUnitBuff()
{
}

void ATWHeroUnitBuff::UseSkill()
{
	UE_LOG(LogTWHero, Warning, TEXT("UseSkill() 호출됨"));

	if (!GetSkillReady())
	{
		UE_LOG(LogTWHero, Warning, TEXT("스킬 사용 불가 (쿨타임)"));
		return;
	}

	static const FString ContextString(TEXT("BuffHeroContext"));
	FTWHeroTableRowBase* SkillData =
		SkillDataTable ? SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString) : nullptr;

	if (!SkillData)
	{
		UE_LOG(LogTWHero, Warning, TEXT("스킬 사용 불가 (데이터 없음)"));
		return;
	}

	// 1) 영웅 자기 자신 버프
	ApplySelfBuff();

	// 2) 주변 아군 일반 유닛(MassEntity) 버프
	if (UTWUnitSubsystem* UnitSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr)
	{
		TArray<ETWStatusType> TargetStatuses = {
			ETWStatusType::Damage,
			ETWStatusType::Armor,
			ETWStatusType::AttackSpeed,
			ETWStatusType::MoveSpeed
		};

		UnitSubsystem->ApplyTemporaryMultiplierBuffToFriendlyUnits(
			GetActorLocation(),
			SkillData->SkillRange,
			GetOwnerPlayerSlot(),
			SkillData->StatMultiplier,
			SkillData->BuffDuration,
			TargetStatuses,
			true
		);
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			ATWPlayerController* TargetPC = Cast<ATWPlayerController>(It->Get());
			if (!TargetPC)
			{
				continue;
			}

			ATWPlayerState* TargetPS = TargetPC->GetPlayerState<ATWPlayerState>();
			if (!TargetPS)
			{
				continue;
			}

			if (TargetPS->PlayerSlot != GetOwnerPlayerSlot())
			{
				continue;
			}

			TargetPC->ClientRefreshSelectedUnitStatusAndUI();
			break;
		}
	}

	CommitSkillCooldown();

	UE_LOG(
		LogTWHero,
		Log,
		TEXT("버프 스킬 시전! Radius=%.1f Multiplier=%.2f Duration=%.1f"),
		SkillData->SkillRange,
		SkillData->StatMultiplier,
		SkillData->BuffDuration
	);

	FTimerHandle BuffTimerHandle;
	GetWorldTimerManager().SetTimer(
		BuffTimerHandle,
		[this]()
		{
			EndSelfBuff();
			UE_LOG(LogTWHero, Log, TEXT("자기 자신 버프 종료"));
		},
		SkillData->BuffDuration,
		false
	);
}

void ATWHeroUnitBuff::ApplySelfBuff()
{
	static const FString ContextString(TEXT("BuffHeroContext"));
	const FTWHeroTableRowBase* SkillData =
		SkillDataTable ? SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString) : nullptr;

	if (!SkillData)
	{
		return;
	}

	ApplyHeroBuffMultiplier(SkillData->StatMultiplier);
}

void ATWHeroUnitBuff::EndSelfBuff()
{
	RestoreHeroBuff();
}