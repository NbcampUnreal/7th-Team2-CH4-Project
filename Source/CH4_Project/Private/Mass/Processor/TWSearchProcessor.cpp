#include "Mass/Processor/TWSearchProcessor.h"

#include "MassCommonTypes.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Mass/Fragments/TWStatusFragment.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Fragments/TWTargetFragment.h"


UTWSearchProcessor::UTWSearchProcessor()
	:EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Movement);
	bRequiresGameThreadExecution = false;
}

void UTWSearchProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWStatusFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTWTargetFragment>(EMassFragmentAccess::ReadWrite);
}

void UTWSearchProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, 
		[this, &EntityManager](FMassExecutionContext& Context)
	{
		const int32 EntityMaxCount = Context.GetNumEntities();
		const auto Transforms = Context.GetFragmentView<FTransformFragment>();
		const auto Range = Context.GetFragmentView<FTWStatusFragment>();
		const auto Targets = Context.GetMutableFragmentView<FTWTargetFragment>();

		for (int32 EntityIdx = 0; EntityIdx < EntityMaxCount; ++EntityIdx)
		{
			// 1. 초기화
			Targets[EntityIdx].bIsTargetInRange = false;  // 일단 false
			FVector MyLoc = Transforms[EntityIdx].GetTransform().GetLocation();  // 현재 유닛의 위치
			
			const FTWUnitStatus& UnitStatus = Range[EntityIdx].GetStatus();
			float AttackRange = UnitStatus.GetStatus(ETWStatusType::Range);
			
			FMassEntityHandle ClosestEnemy;  // 가장 가까운 엔티티 저장
			float ClosestDistSq = FMath::Square(AttackRange);  // 가장 가까운 엔티티와의 거리

			// TODO: 공간 해시 그리드에서 주변 엔티티 핸들 목록 가져오기 로직
            
			// 2. 만약 적을 찾았다면?
			if (ClosestEnemy.IsValid())
			{
				Targets[EntityIdx].TargetEntity = ClosestEnemy;  // 해당 엔티티를 저장
				Targets[EntityIdx].bIsTargetInRange = true;  // 발견 판정
			}
		}
	});
}