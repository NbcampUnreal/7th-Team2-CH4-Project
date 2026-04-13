#pragma once

#include "CoreMinimal.h"
#include "MassStateTreeTypes.h"
#include "TWTargetEvaluator.generated.h"

struct FTWAttackFragment;
struct FTWTargetFragment;

USTRUCT()
struct FTWTargetEvaluatorInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Target")
	bool bIsEnemyInRange;
	
};

USTRUCT()
struct FTWTargetEvaluator : public FMassStateTreeEvaluatorBase
{
	GENERATED_BODY()
	using FInstanceDataType = FTWTargetEvaluatorInstanceData;
protected:
	TStateTreeExternalDataHandle<FTWAttackFragment>AttackHandle;
	TStateTreeExternalDataHandle<FTWTargetFragment>TargetHandle;
	
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
