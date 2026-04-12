#pragma once

#include "CoreMinimal.h"
#include "MassNavigationTypes.h"
#include "MassStateTreeTypes.h"
#include "TWStateTreeStopTask.generated.h"

struct FTransformFragment;
struct FTWStandFragment;
struct FMassMoveTargetFragment;

USTRUCT()
struct CH4_PROJECT_API FTWMassStopInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = Output)
	FMassTargetLocation TargetLocation;
};

USTRUCT()
struct FTWStateTreeStopTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FTWMassStopInstanceData;
	
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override
	{
		return FTWMassStopInstanceData::StaticStruct();
	}
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context, 
		const float DeltaTime) const override;
	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	
protected:
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
	TStateTreeExternalDataHandle<FTWStandFragment> StandHandle;
};
