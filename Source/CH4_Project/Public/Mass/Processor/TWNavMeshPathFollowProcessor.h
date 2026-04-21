// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "MassNavigationTypes.h"
#include "MassNavMeshNavigationFragments.h"
#include "MassSignalProcessorBase.h"
#include "TWNavMeshPathFollowProcessor.generated.h"

struct FTWStatusFragment;

UCLASS()
class CH4_PROJECT_API UTWNavMeshPathFollowProcessor : public UMassSignalProcessorBase
{
	GENERATED_BODY()

public:
	UTWNavMeshPathFollowProcessor(const FObjectInitializer& ObjectInitializer);

protected:
	FMassEntityQuery EntityQuery;

	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context,
	                            FMassSignalNameLookup& EntitySignals) override;
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;

private:
	bool RequestPath(
		const FVector& TargetLocation,
		const UWorld* World,
		const FAgentRadiusFragment& AgentRadius,
		const FVector AgentNavLocation,
		FMassNavMeshCachedPathFragment& CachedPathFragment,
		FMassNavMeshShortPathFragment& ShortPathFragment,
		FMassMoveTargetFragment& MoveTarget,
		const FMassMovementParameters& MovementParams,
		const FMassDesiredMovementFragment& DesiredMovementFragment,
		const FTWStatusFragment& StatusFragment

	) const;
};
