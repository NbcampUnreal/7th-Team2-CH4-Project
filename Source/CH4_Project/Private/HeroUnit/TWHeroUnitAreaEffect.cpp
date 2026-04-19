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
			// 1. 사거리 가이드 크기 조절 (반지름 50 메시 기준)
			// 만약 SkillRange가 500이면 Scale은 10이 되어 반지름 500인 원이 됩니다.
			float RangeScale = SkillData->SkillRange / 50.f;
			SkillRangeMesh->SetRelativeScale3D(FVector(RangeScale, RangeScale, 1.f));
           
			// 2. 다이나믹 머티리얼 생성 및 할당
			// 반투명하거나 테두리만 있는 사거리 전용 머티리얼을 인스턴스 0번에 넣어두세요.
			UMaterialInstanceDynamic* DynMaterial = SkillRangeMesh->CreateDynamicMaterialInstance(0);
			if (DynMaterial)
			{
				// 필요하다면 여기서 사거리 원의 색상을 별도로 지정할 수 있습니다.
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

	// IndicatorRadius 변수가 수정되었을 때 실시간으로 메시 크기 변경
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ATWHeroUnitAreaEffect, IndicatorRadius))
	{
		UpdateMeshScale();
	}
}
#endif

void ATWHeroUnitAreaEffect::UseSkill()
{
	if (TryStartCooldown())
	{
		// FVector SkillLocation = CurrentTargetLocation;
		
		if (DynamicIndicatorMaterial)
		{
			DynamicIndicatorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
		}
		
		FTimerHandle WarningTimerHandle;
		GetWorldTimerManager().SetTimer(WarningTimerHandle, [this]()
		{
			if (DynamicIndicatorMaterial)
			{
				// 여기에 공격판정을 넣으면 됨
				UE_LOG(LogTemp, Warning, TEXT("메테오 폭발!!"));
				
				// 공격판정 된느지 확인하려고 넣은 매쉬색상 변경 없어도됨
				DynamicIndicatorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Red);
			}
			
			// 공격 판정이 일어난 구역 확인하려고 넣은 타이머 없애버리면 됨
			FTimerHandle ClearTimerHandle;
			GetWorldTimerManager().SetTimer(ClearTimerHandle, [this]()
			{
				SetIndicatorVisible(false);
				if (DynamicIndicatorMaterial)
				{
					DynamicIndicatorMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor::Green);
				}
			}, 1.0f, false);

		}, 3.0f, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("스킬 사용 불가 (쿨타임 or 데이터 찾을 수 없음)"));
	}
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



