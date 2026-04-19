#pragma once

#include "CoreMinimal.h"
#include "HeroUnit/TWHeroUnitBase.h"
#include "TWHeroUnitBuff.generated.h"

UCLASS()
class CH4_PROJECT_API ATWHeroUnitBuff : public ATWHeroUnitBase
{
	GENERATED_BODY()
	
public:
	ATWHeroUnitBuff();
	
	virtual void UseSkill() override;
	virtual uint8 IsIndicatorRequired() const override {return false;}
	virtual uint8 IsRangeRequired() const override {return false;}
	
protected:
	void ApplySelfBuff();
	void EndSelfBuff();
};
