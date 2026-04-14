#include "Mass/StateTree/TWStateTreeStopTask.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "Mass/Fragments/TWStandFragment.h"
#include "MassMovementFragments.h"

bool FTWStateTreeStopTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	Linker.LinkExternalData(StandHandle);
	// 기본 엔진에서 제공하는 속도
	Linker.LinkExternalData(VelocityHandle);
	return true;
}

EStateTreeRunStatus FTWStateTreeStopTask::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UE_LOG(LogTemp, Warning, TEXT("Stop Task Entered!"));
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	FMassVelocityFragment& Velocity = Context.GetExternalData(VelocityHandle);
	
	const FTransform& CurrentTransform = Context.GetExternalData(TransformHandle).GetTransform();
	InstanceData.TargetLocation.EndOfPathPosition = CurrentTransform.GetLocation();
	
	FTWStandFragment& StandFragment = Context.GetExternalData(StandHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	
	StandFragment.bIsStopping = true;
	
	MoveTarget.DesiredSpeed.Set(0.0f);
	Velocity.Value = FVector::ZeroVector;
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
	
	UE_LOG(LogMass, Log, TEXT("Exit Stop State  <<================"));
	FMassStateTreeTaskBase::ExitState(Context, Transition);
}
