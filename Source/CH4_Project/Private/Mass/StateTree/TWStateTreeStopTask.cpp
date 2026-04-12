#include "Mass/StateTree/TWStateTreeStopTask.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "Mass/Fragments/TWStandFragment.h"

bool FTWStateTreeStopTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(StandHandle);
	return true;
}

EStateTreeRunStatus FTWStateTreeStopTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	const FTransform& CurrentTransform = Context.GetExternalData(TransformHandle).GetTransform();
	InstanceData.TargetLocation.EndOfPathPosition = CurrentTransform.GetLocation();
	
	FTWStandFragment& StandFragment = Context.GetExternalData(StandHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	
	StandFragment.bIsStopping = true;
	
	MoveTarget.DesiredSpeed.Set(0.0f);
	MoveTarget.CreateNewAction(EMassMovementAction::Stand, *Context.GetWorld());
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FTWStateTreeStopTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return FMassStateTreeTaskBase::Tick(Context, DeltaTime);
}

void FTWStateTreeStopTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FTWStandFragment& StandFragment = Context.GetExternalData(StandHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	
	StandFragment.bIsStopping = false;
	
	MoveTarget.CreateNewAction(EMassMovementAction::Move, *Context.GetWorld());
	
	UE_LOG(LogMass, Log, TEXT("Exit Stop State  <<================"));
	FMassStateTreeTaskBase::ExitState(Context, Transition);
}
