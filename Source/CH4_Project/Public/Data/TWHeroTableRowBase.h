#pragma once

#include "CoreMinimal.h"
#include "TWHeroTableRowBase.generated.h"

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTWHeroTableRowBase : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	FName HeroSkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float SkillCooldown = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float SkillRange = 5.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	TSubclassOf<AActor> ProjectileClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float StatMultiplier = 1.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float BuffDuration = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero|Skill")
	float SkillMultiplier = 1.0f;
};
