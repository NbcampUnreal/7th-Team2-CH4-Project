#include "HeroUnit/TWHeroUnitBase.h"

#include "Data/TWHeroTableRowBase.h"
#include "Data/TWUnitTableRowBase.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Building/TWBaseBuilding.h"
#include "Core/TWGameMode.h"
#include "Core/TWPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

ATWHeroUnitBase::ATWHeroUnitBase()
{
	bIsSkillReady = true;
	CurrentHeroHP = 0.f;
	bHeroBuffApplied = false;
	CurrentTargetLocation = FVector::ZeroVector;
	bHeroDead = false;
	bRespawnRequested = false;
}

void ATWHeroUnitBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[HeroActor BeginPlay] Name=%s NetMode=%d HasAuthority=%s World=%s"),
		*GetName(),
		(int32)GetNetMode(),
		HasAuthority() ? TEXT("true") : TEXT("false"),
		*GetWorld()->GetName()
	);

	const UTWUnitSubsystem* UnitSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UTWUnitSubsystem>() : nullptr;

	const FTWUnitTableRowBase* UnitRow =
		UnitSubsystem ? UnitSubsystem->GetUnitTableRowBase(SkillOwnHero) : nullptr;

	if (UnitRow)
	{
		BaseHeroStatus = UnitRow->BaseStatus;
		RuntimeHeroStatus = BaseHeroStatus;
		CurrentHeroHP = RuntimeHeroStatus.GetStatus(ETWStatusType::Health);
	}
	else
	{
		BaseHeroStatus = FTWUnitStatus();
		BaseHeroStatus.SetStatus(ETWStatusType::Health, 100.f);
		BaseHeroStatus.SetStatus(ETWStatusType::Damage, 10.f);
		BaseHeroStatus.SetStatus(ETWStatusType::Armor, 0.f);
		BaseHeroStatus.SetStatus(ETWStatusType::AttackSpeed, 1.f);
		BaseHeroStatus.SetStatus(ETWStatusType::MoveSpeed, 300.f);

		RuntimeHeroStatus = BaseHeroStatus;
		CurrentHeroHP = RuntimeHeroStatus.GetStatus(ETWStatusType::Health);
	}
}

uint8 ATWHeroUnitBase::TryStartCooldown()
{
	if (!bIsSkillReady || !SkillDataTable || SkillOwnHero.IsNone())
	{
		return false;
	}

	static const FString ContextString(TEXT("SkillCooldownContext"));
	FTWHeroTableRowBase* Row = SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString);

	if (!Row)
	{
		return false;
	}

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

void ATWHeroUnitBase::HandleSkillReady()
{
	bIsSkillReady = true;
	OnSkillReady();
}

float ATWHeroUnitBase::GetRemainingCooldownTime() const
{
	if (bIsSkillReady)
	{
		return 0.f;
	}

	return GetWorldTimerManager().GetTimerRemaining(SkillCooldownTimerHandle);
}

float ATWHeroUnitBase::GetCooldownPercentage() const
{
	if (bIsSkillReady)
	{
		return 0.f;
	}

	if (!GetWorldTimerManager().IsTimerActive(SkillCooldownTimerHandle))
	{
		return 0.f;
	}

	static const FString ContextString(TEXT("SkillCooldownContext"));
	const FTWHeroTableRowBase* Row =
		SkillDataTable ? SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString) : nullptr;

	if (!Row || Row->SkillCooldown <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float RemainingCooldownTime = GetRemainingCooldownTime();
	return FMath::Clamp(RemainingCooldownTime / Row->SkillCooldown, 0.f, 1.f);
}

bool ATWHeroUnitBase::CommitSkillCooldown()
{
	return TryStartCooldown() ? true : false;
}

float ATWHeroUnitBase::GetHeroDamageStat() const
{
	return RuntimeHeroStatus.GetStatus(ETWStatusType::Damage);
}

ATWPlayerState* ATWHeroUnitBase::ResolveOwnerPlayerState() const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	if (const AController* OwnerController = GetOwner<AController>())
	{
		if (ATWPlayerState* OwnerPS = OwnerController->GetPlayerState<ATWPlayerState>())
		{
			if (OwnerPS->PlayerSlot == GetOwnerPlayerSlot())
			{
				return OwnerPS;
			}
		}
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
		if (!PS)
		{
			continue;
		}

		if (PS->PlayerSlot == GetOwnerPlayerSlot())
		{
			return PS;
		}
	}

	return nullptr;
}

void ATWHeroUnitBase::TryRequestHeroRespawn()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bRespawnRequested)
	{
		return;
	}

	ATWPlayerState* OwnerPS = ResolveOwnerPlayerState();
	if (!OwnerPS)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[%s] TryRequestHeroRespawn failed: OwnerPS not found | OwnerSlot=%d"),
			*GetName(),
			GetOwnerPlayerSlot()
		);
		return;
	}

	if (OwnerPS->IsHeroRespawnPending())
	{
		return;
	}

	ATWGameMode* GM = GetWorld() ? GetWorld()->GetAuthGameMode<ATWGameMode>() : nullptr;
	if (!GM)
	{
		return;
	}

	bRespawnRequested = true;
	OwnerPS->SetHeroRespawnPending(true);

	GM->RequestRespawnHero(
		OwnerPS,
		SkillOwnHero,
		OwnerPS->GetHeroRespawnDelay()
	);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[%s] Hero respawn requested | OwnerSlot=%d | HeroId=%s"),
		*GetName(),
		GetOwnerPlayerSlot(),
		*SkillOwnHero.ToString()
	);
}

void ATWHeroUnitBase::HandleHeroDeath(AActor* DamageCauser)
{
	if (bHeroDead)
	{
		return;
	}

	bHeroDead = true;

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[%s] Hero died | OwnerSlot=%d | HeroId=%s | Causer=%s"),
		*GetName(),
		GetOwnerPlayerSlot(),
		*SkillOwnHero.ToString(),
		DamageCauser ? *DamageCauser->GetName() : TEXT("None")
	);

	TryRequestHeroRespawn();

	Destroy();
}

void ATWHeroUnitBase::ReceiveHeroDamage(float InDamageAmount, AActor* DamageCauser)
{
	if (InDamageAmount <= 0.f || CurrentHeroHP <= 0.f || bHeroDead)
	{
		return;
	}

	CurrentHeroHP = FMath::Max(0.f, CurrentHeroHP - InDamageAmount);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[%s] Hero Damage Applied: %.1f / CurrentHP: %.1f / Causer: %s"),
		*GetName(),
		InDamageAmount,
		CurrentHeroHP,
		DamageCauser ? *DamageCauser->GetName() : TEXT("None")
	);

	if (CurrentHeroHP <= 0.f)
	{
		HandleHeroDeath(DamageCauser);
	}
}

bool ATWHeroUnitBase::IsSameTeamActor(const AActor* OtherActor) const
{
	if (!OtherActor)
	{
		return false;
	}

	if (const ATWUnit* OtherUnit = Cast<ATWUnit>(OtherActor))
	{
		return OtherUnit->GetOwnerPlayerSlot() == GetOwnerPlayerSlot();
	}

	if (const ATWBaseBuilding* OtherBuilding = Cast<ATWBaseBuilding>(OtherActor))
	{
		return OtherBuilding->GetOwnerPlayerSlot() == GetOwnerPlayerSlot();
	}

	return false;
}

bool ATWHeroUnitBase::TryApplyDamageToActor(AActor* TargetActor, float DamageAmount)
{
	if (!TargetActor || DamageAmount <= 0.f)
	{
		return false;
	}

	if (TargetActor == this)
	{
		return false;
	}

	if (IsSameTeamActor(TargetActor))
	{
		return false;
	}

	if (ATWHeroUnitBase* TargetHero = Cast<ATWHeroUnitBase>(TargetActor))
	{
		TargetHero->ReceiveHeroDamage(DamageAmount, this);
		return true;
	}

	if (ATWBaseBuilding* TargetBuilding = Cast<ATWBaseBuilding>(TargetActor))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[%s] Building target ignored by hero skill damage: %s"),
			*GetName(),
			*TargetBuilding->GetName()
		);
		return false;
	}

	return false;
}

void ATWHeroUnitBase::ApplyHeroBuffMultiplier(float InMultiplier)
{
	if (bHeroBuffApplied || InMultiplier <= 1.0f)
	{
		return;
	}

	CachedPreBuffStatus = RuntimeHeroStatus;

	auto MultiplyStatus = [this, InMultiplier](ETWStatusType StatusType)
	{
		if (StatusType == ETWStatusType::Health)
		{
			return;
		}

		const float OldValue = RuntimeHeroStatus.GetStatus(StatusType);
		RuntimeHeroStatus.SetStatus(StatusType, OldValue * InMultiplier);
	};

	MultiplyStatus(ETWStatusType::Damage);
	MultiplyStatus(ETWStatusType::Armor);
	MultiplyStatus(ETWStatusType::AttackSpeed);
	MultiplyStatus(ETWStatusType::MoveSpeed);

	bHeroBuffApplied = true;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[%s] Self Buff Applied: Damage=%.1f Armor=%.1f AttackSpeed=%.1f MoveSpeed=%.1f"),
		*GetName(),
		RuntimeHeroStatus.GetStatus(ETWStatusType::Damage),
		RuntimeHeroStatus.GetStatus(ETWStatusType::Armor),
		RuntimeHeroStatus.GetStatus(ETWStatusType::AttackSpeed),
		RuntimeHeroStatus.GetStatus(ETWStatusType::MoveSpeed)
	);
}

void ATWHeroUnitBase::RestoreHeroBuff()
{
	if (!bHeroBuffApplied)
	{
		return;
	}

	RuntimeHeroStatus = CachedPreBuffStatus;
	bHeroBuffApplied = false;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[%s] Self Buff Restored"),
		*GetName()
	);
}