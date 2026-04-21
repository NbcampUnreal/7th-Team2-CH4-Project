#include "HeroUnit/TWHeroUnitAreaEffect.h"

#include "Data/TWHeroTableRowBase.h"

ATWHeroUnitAreaEffect::ATWHeroUnitAreaEffect()
{
	AreaIndicatorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AreaIndicator"));
	AreaIndicatorMesh->SetupAttachment(RootComponent);
	
	AreaIndicatorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AreaIndicatorMesh->SetVisibility(false);
	AreaIndicatorMesh->SetCastShadow(false);
	AreaIndicatorMesh->SetComponentTickEnabled(false);
	
	SkillRangeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkillRangeMesh")); // 화살표(->)를 이퀄(=)로 변경
	SkillRangeMesh->SetupAttachment(RootComponent);
    
	SkillRangeMesh->SetVisibility(false);
	SkillRangeMesh->SetCastShadow(false);
}

void ATWHeroUnitAreaEffect::BeginPlay()
{
	Super::BeginPlay();
	
	if (AreaIndicatorMesh)
	{
		DynamicIndicatorMaterial = AreaIndicatorMesh->CreateDynamicMaterialInstance(0);
		
		UpdateMeshScale();
		DynamicIndicatorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
	}
	
	if (SkillRangeMesh && SkillDataTable)
	{
		static const FString ContextString(TEXT("AreaEffectHeroContext"));
		FTWHeroTableRowBase* SkillData = SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString);
		
		if (SkillData)
		{
			float RangeScale = SkillData->SkillRange / 50.f;
			SkillRangeMesh->SetRelativeScale3D(FVector(RangeScale, RangeScale, 1.f));
           
			UMaterialInstanceDynamic* DynMaterial = SkillRangeMesh->CreateDynamicMaterialInstance(0);
			if (DynMaterial)
			{
				DynMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::White);
			}
		}
	}
}


#if WITH_EDITOR
void ATWHeroUnitAreaEffect::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ATWHeroUnitAreaEffect, IndicatorRadius))
	{
		UpdateMeshScale();
	}
}
#endif

void ATWHeroUnitAreaEffect::UseSkill()
{
	if (!GetSkillReady())
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (쿨타임)"));
		return;
	}

	if (!DynamicIndicatorMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (Indicator Material 없음)"));
		return;
	}

	// 실게임의 실제 데미지 판정은 PlayerController -> UnitSubsystem 서버 로직에서 처리
	// 여기서는 범위 표시 연출만 담당
	DynamicIndicatorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Red);

	FTimerHandle ClearTimerHandle;
	GetWorldTimerManager().SetTimer(
		ClearTimerHandle,
		[this]()
		{
			SetIndicatorVisible(false);
			SetRangeVisible(false);

			if (DynamicIndicatorMaterial)
			{
				DynamicIndicatorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
			}
		},
		0.35f,
		false
	);

	CommitSkillCooldown();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Astrologian area effect visual triggered at %s"),
		*CurrentTargetLocation.ToString()
	);
}

void ATWHeroUnitAreaEffect::UpdateIndicator(FVector MouseLocation)
{
	static const FString ContextString(TEXT("AreaEffectHeroContext"));
	FTWHeroTableRowBase* SkillData = SkillDataTable->FindRow<FTWHeroTableRowBase>(SkillOwnHero, ContextString);
	
	FVector HeroLocation = GetActorLocation();
	float MaxRange = SkillData->SkillRange;
	
	FVector Direction = MouseLocation - HeroLocation;
	Direction.Z = 0.f;
    
	float Distance = Direction.Size();
	
	if (Distance > MaxRange)
	{
		Direction.Normalize();
		CurrentTargetLocation = HeroLocation + (Direction * MaxRange);
	}
	else
	{
		CurrentTargetLocation = MouseLocation;
	}
	
	if (AreaIndicatorMesh && AreaIndicatorMesh->IsVisible())
	{
		FVector NewLocation = CurrentTargetLocation + FVector(0.f, 0.f, IndicatorZ);
		AreaIndicatorMesh->SetWorldLocation(NewLocation);
	}
}

void ATWHeroUnitAreaEffect::SetIndicatorVisible(bool bIsVisible)
{
	if (AreaIndicatorMesh)
	{
		AreaIndicatorMesh->SetVisibility(bIsVisible);
	}
}

void ATWHeroUnitAreaEffect::SetRangeVisible(bool bIsVisible)
{
	if (AreaIndicatorMesh)
	{
		SkillRangeMesh->SetVisibility(bIsVisible);
	}
}

void ATWHeroUnitAreaEffect::UpdateMeshScale()
{
	if (AreaIndicatorMesh)
	{
		float ScaleValue = IndicatorRadius / 50.f;
		AreaIndicatorMesh->SetRelativeScale3D(FVector(ScaleValue, ScaleValue, 1.f));
	}
}
