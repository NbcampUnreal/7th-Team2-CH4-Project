// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassProcessor.h"
#include "Mass/Fragments/TWTransformOffsetFragment.h"
#include "Mass/Fragments/TWTransformOffsetParams.h"
#include "TWTransformSmoothOffsetProcessor.generated.h"

/**
 * 
 */

UCLASS()
class CH4_PROJECT_API UTWTransformSmoothOffsetProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UTWTransformSmoothOffsetProcessor()
	{
		bAutoRegisterWithProcessingPhases = true;
		ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::Client);
		ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	}

	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override
	{
		EntityQuery.Initialize(EntityManager);
		EntityQuery.AddRequirement<FTWTransformOffsetFragment>(EMassFragmentAccess::ReadWrite);
		EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
		EntityQuery.AddConstSharedRequirement<FTWTransformOffsetParams>();
		EntityQuery.RegisterWithProcessor(*this);
	}

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override
	{
		EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
		{
			const TArrayView<FTWTransformOffsetFragment>& TranslationOffsets = Context.GetMutableFragmentView<
				FTWTransformOffsetFragment>();
			const FTWTransformOffsetParams& Params = Context.GetConstSharedFragment<FTWTransformOffsetParams>();

			const float DeltaTime = Context.GetWorld()->DeltaTimeSeconds;
			for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
			{
				FTWTransformOffsetFragment& TranslationOffset = TranslationOffsets[EntityIndex];
				if (DeltaTime < Params.MaxTimeToSmooth)
				{
					TranslationOffset.TransformOffset.SetPosition(
						TranslationOffset.TransformOffset.GetPosition() * (1.0f - DeltaTime / Params.SmoothTime));
					TranslationOffset.TransformOffset.SetYaw(
						TranslationOffset.TransformOffset.GetYaw() * (1.0f - DeltaTime / Params.SmoothTime));

					//
					// FVector CurrentPos = TranslationOffset.TransformOffset.GetPosition();
					// float Distance = CurrentPos.Size();
					//
					// if (Distance > 0.0f)
					// {
					// 	float InterpolationSpeed = 100.0f / FMath::Max(Params.SmoothTime, 0.001f);
					// 	FVector NewPos = FMath::VInterpConstantTo(CurrentPos, FVector::ZeroVector, DeltaTime,
					// 	                                          InterpolationSpeed);
					// 	TranslationOffset.TransformOffset.SetPosition(NewPos);
					// }
					//
					// float CurrentYaw = TranslationOffset.TransformOffset.GetYaw();
					// float YawSpeed = 180.0f / FMath::Max(Params.SmoothTime, 0.001f);
					// float NewYaw = FMath::FInterpConstantTo(CurrentYaw, 0.0f, DeltaTime, YawSpeed);
					// TranslationOffset.TransformOffset.SetYaw(NewYaw);
				}
				else
				{
					TranslationOffset.TransformOffset.SetPosition(FVector::ZeroVector);
					TranslationOffset.TransformOffset.SetYaw(0.0f);
				}
			}
		});
	}

private:
	FMassEntityQuery EntityQuery;
};
