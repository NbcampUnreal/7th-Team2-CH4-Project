#include "Mass/StateTree/TWStateTreeStopTask.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "MassMovementFragments.h"

bool FTWStateTreeStopTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	return true;
}

EStateTreeRunStatus FTWStateTreeStopTask::EnterState(
	FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UE_LOG(LogTemp, Warning, TEXT("Stop Task Entered!"));
	const FVector MyCurrentLocation = Context.GetExternalData(TransformHandle).GetTransform().GetLocation();
	
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	MoveTarget.Center = MyCurrentLocation;
	
	MoveTarget.DistanceToGoal = 0.0f;
	MoveTarget.DesiredSpeed.Set(0.0f);
	MoveTarget.IntentAtGoal = EMassMovementAction::Stand;
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FTWStateTreeStopTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return FMassStateTreeTaskBase::Tick(Context, DeltaTime);
}

void FTWStateTreeStopTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	
	UE_LOG(LogMass, Log, TEXT("Exit Stop State  <<================"));
	FMassStateTreeTaskBase::ExitState(Context, Transition);
}
