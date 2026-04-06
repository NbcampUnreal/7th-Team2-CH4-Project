// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/StateTree/TESTStateTreeTask.h"

#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FTESTStateTreeTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityCommandDataHandle);
	Linker.LinkExternalData(EntityCommandTypeHandle);
	return true;
}

EStateTreeRunStatus FTESTStateTreeTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FCommandDataFragment& CommandData = Context.GetExternalData(EntityCommandDataHandle);
	InstanceData.TargetLocation.EndOfPathPosition = CommandData.GetLocation();
	
	FCommandTypeFragment& CommandType = Context.GetExternalData(EntityCommandTypeHandle);
	CommandType.SetType(ECommandType::None);
	
	UE_LOG(LogMass, Warning, TEXT("%lf, %lf, %lf"), CommandData.GetLocation().X,CommandData.GetLocation().Y,CommandData.GetLocation().Z)
	return EStateTreeRunStatus::Running;
	
}

EStateTreeRunStatus FTESTStateTreeTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return FMassStateTreeTaskBase::Tick(Context, DeltaTime);
}

void FTESTStateTreeTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{

	Context.GetMutableEventQueue().Reset();
	FMassStateTreeTaskBase::ExitState(Context, Transition);
}