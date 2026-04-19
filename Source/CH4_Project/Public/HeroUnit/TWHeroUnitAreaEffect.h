#pragma once

#include "CoreMinimal.h"
#include "HeroUnit/TWHeroUnitBase.h"
#include "TWHeroUnitAreaEffect.generated.h"

UCLASS()
class CH4_PROJECT_API ATWHeroUnitAreaEffect : public ATWHeroUnitBase
{
	GENERATED_BODY()
	
public:
	ATWHeroUnitAreaEffect();
	
	virtual void BeginPlay() override;
	
	virtual void UseSkill() override;
	
	virtual void UpdateIndicator(FVector MouseLocation) override;
	virtual void SetIndicatorVisible(bool bIsVisible) override;
	virtual void SetRangeVisible(bool Visible) override;
	virtual uint8 IsIndicatorRequired() const override {return true;}
	virtual uint8 IsRangeRequired() const override {return true;}
	
#pragma region Indicator
	
	void UpdateMeshScale();
	
#if WITH_EDITOR
	// 에디터에서 수치 변경 시 즉시 반영하기 위한 함수
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	UPROPERTY(EditAnywhere, Category="Hero|UI")
	TObjectPtr<UStaticMeshComponent> AreaIndicatorMesh;
	
	UPROPERTY(EditAnywhere, Category="Hero|UI")
	TObjectPtr<UStaticMeshComponent> SkillRangeMesh;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicIndicatorMaterial;
	
	UPROPERTY(EditAnywhere, Category="Hero|UI")
	float IndicatorZ = 0.f;
	
	UPROPERTY(EditAnywhere, Category="Hero|UI")
	float IndicatorRadius = 500.f;
	
#pragma endregion
};
