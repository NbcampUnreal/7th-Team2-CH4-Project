#pragma once

#include "CoreMinimal.h"
#include "HeroUnit/TWHeroUnitBase.h"
#include "TWHeroUnitProjectile.generated.h"

UCLASS()
class CH4_PROJECT_API ATWHeroUnitProjectile : public ATWHeroUnitBase
{
	GENERATED_BODY()
	
public:
	ATWHeroUnitProjectile();
	
	virtual void UseSkill() override;
	
	virtual void UpdateIndicator(FVector MouseLocation) override;
	virtual void SetIndicatorVisible(bool bIsVisible) override;
	virtual uint8 IsIndicatorRequired() const override {return true;}
	virtual uint8 IsRangeRequired() const override {return false;}
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ProjectileSpawnPoint;
	
	UPROPERTY(EditAnywhere, Category="Hero|Skill")
	float ProjectileZ = 0.f;
	
#pragma region Indicator
public:
	
	
protected:
	UPROPERTY(EditAnywhere, Category="Hero|UI")
	TObjectPtr<UStaticMeshComponent> AimIndicatorMesh;
	
	UPROPERTY(EditAnywhere, Category="Hero|UI")
	float IndicatorZ = 0.f;
	
#pragma endregion
};
