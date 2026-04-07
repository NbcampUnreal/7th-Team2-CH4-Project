// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/StateTree/TWStateTreeCommandTask.h"

#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FTWStateTreeCommandTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(EntityCommandDataHandle);
	Linker.LinkExternalData(EntityCommandTypeHandle);
	return true;
}

EStateTreeRunStatus FTWStateTreeCommandTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FTWCommandDataFragment& CommandData = Context.GetExternalData(EntityCommandDataHandle);
	InstanceData.TargetLocation.EndOfPathPosition = CommandData.GetLocation();
	
	FTWCommandTypeFragment& CommandType = Context.GetExternalData(EntityCommandTypeHandle);
	CommandType.SetType(ETWCommandType::None);
	
	UE_LOG(LogMass, Warning, TEXT("%lf, %lf, %lf"), CommandData.GetLocation().X,CommandData.GetLocation().Y,CommandData.GetLocation().Z)
	return EStateTreeRunStatus::Running;
	
}

EStateTreeRunStatus FTWStateTreeCommandTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return FMassStateTreeTaskBase::Tick(Context, DeltaTime);
}

void FTWStateTreeCommandTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{

	Context.GetMutableEventQueue().Reset();
	FMassStateTreeTaskBase::ExitState(Context, Transition);
}