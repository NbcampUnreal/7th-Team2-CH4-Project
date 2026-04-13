#include "Mass/StateTree/TWTargetEvaluator.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWTargetFragment.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FTWTargetEvaluator::Link(FStateTreeLinker& Linker)
{
	// 스키마 에러를 방지하기 위해 데이터를 링크합니다.
	Linker.LinkExternalData(AttackHandle);
	Linker.LinkExternalData(TargetHandle);
	return true;
}

void FTWTargetEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData =  Context.GetInstanceData(*this);
    
	const FTWAttackFragment& AttackRange = Context.GetExternalData(AttackHandle);
	const FTWTargetFragment& AttackTarget = Context.GetExternalData(TargetHandle);
    
	
	
}