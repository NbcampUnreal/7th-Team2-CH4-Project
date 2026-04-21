// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "SmoothOrientation/MassSmoothOrientationFragments.h"
#include "TWOrientationProcessor.generated.h"

UCLASS()
class CH4_PROJECT_API UTWOrientationProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UTWOrientationProcessor();

protected:
	FMassEntityQuery EntityQuery;
	
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
	
	UPROPERTY(EditAnywhere, Category = "Orientation", meta = (ClampMin = "0.0", ClampMax="1.0"))
	float EndOfPathDuration = 1.0f;
	/** The time it takes the orientation to catchup to the requested orientation. */
	
	UPROPERTY(EditAnywhere, Category = "Orientation", meta = (ClampMin = "0.0", ForceUnits="s"))
	float OrientationSmoothingTime = 0.2f;

	/* Orientation blending weights while moving. */	
	UPROPERTY(EditAnywhere, Category = "Orientation")
	FMassSmoothOrientationWeights Moving = FMassSmoothOrientationWeights(/*MoveTarget*/1.0f, /*Velocity*/0.0f);

	/* Orientation blending weights while standing. */	
	UPROPERTY(EditAnywhere, Category = "Orientation")
	FMassSmoothOrientationWeights Standing = FMassSmoothOrientationWeights(/*MoveTarget*/0.95f, /*Velocity*/0.05f);
};
