#include "HeroUnit/TWHeroUnitProjectile.h"

#include "Data/TWHeroTableRowBase.h"

ATWHeroUnitProjectile::ATWHeroUnitProjectile()
{
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(RootComponent);
	
	AimIndicatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AimIndicatorMesh"));
	AimIndicatorMesh->SetupAttachment(RootComponent);
	AimIndicatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AimIndicatorMesh->SetVisibility(false);
}

void ATWHeroUnitProjectile::UseSkill()
{
	UE_LOG(LogTemp, Warning, TEXT("UseSkill() 호출됨"));

	if (!GetSkillReady())
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (쿨타임)"));
		return;
	}

	static const FString ContextString(TEXT("ProjectileHeroContext"));
	FTWHeroTableRowBase* SkillData =
		SkillDataTable ? SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString) : nullptr;

	if (!SkillData || !SkillData->ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (데이터/ProjectileClass 없음)"));
		return;
	}

	if (!ProjectileSpawnPoint || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (SpawnPoint/World 없음)"));
		return;
	}

	const FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();

	FVector FireDir = CurrentTargetLocation - SpawnLocation;
	FireDir.Z = ProjectileZ;

	AActor* SpawnedProjectile =
		GetWorld()->SpawnActor<AActor>(SkillData->ProjectileClass, SpawnLocation, FireDir.Rotation());

	if (!SpawnedProjectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("투사체 생성 실패"));
		return;
	}

	SetIndicatorVisible(false);
	CommitSkillCooldown();

	UE_LOG(LogTemp, Warning, TEXT("투사체 생성 및 쿨다운 시작"));
}

void ATWHeroUnitProjectile::UpdateIndicator(FVector MouseLocation)
{
	if (AimIndicatorMesh && AimIndicatorMesh->IsVisible())
	{
		CurrentTargetLocation = MouseLocation;
		
		FVector Dir = (CurrentTargetLocation - GetActorLocation());
		Dir.Z = IndicatorZ;
		
		if (!Dir.IsNearlyZero())
		{
			AimIndicatorMesh->SetWorldRotation(Dir.Rotation());
		}
	}
}

void ATWHeroUnitProjectile::SetIndicatorVisible(bool bIsVisible)
{
	if (AimIndicatorMesh)
	{
		AimIndicatorMesh->SetVisibility(bIsVisible);
	}
}
