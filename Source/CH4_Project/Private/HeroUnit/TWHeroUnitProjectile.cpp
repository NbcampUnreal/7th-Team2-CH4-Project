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
	if (TryStartCooldown())
	{
		SetIndicatorVisible(false);
		
		static const FString ContextString(TEXT("ProjectileHeroContext"));
		FTWHeroTableRowBase* SkillData = SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString);
		UE_LOG(LogTemp, Warning, TEXT("스킬데이터 불러와졌습니다."));
		
		if (SkillData && SkillData->ProjectileClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("투사체 생성합니다."));
			FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
			
			FVector FireDir = CurrentTargetLocation - SpawnLocation;
			FireDir.Z = ProjectileZ;
			
			GetWorld()->SpawnActor<AActor>(SkillData->ProjectileClass, SpawnLocation, FireDir.Rotation());
			
			SetIndicatorVisible(false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (쿨타임 or 데이터 찾을 수 없음)"));
	}
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
