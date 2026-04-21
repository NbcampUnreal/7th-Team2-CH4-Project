// Copyright Epic Games, Inc. All Rights Reserved.

#include "Mass/Processor/TWOrientationProcessor.h"
#include "SmoothOrientation/MassSmoothOrientationFragments.h"
#include "MassCommandBuffer.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassNavigationFragments.h"
#include "MassMovementFragments.h"
#include "Math/UnrealMathUtility.h"
#include "MassSimulationLOD.h"
#include "MassNavigationUtils.h"
#include "Mass/Fragments/TWAttackFragment.h"
#include "Mass/Traits/TWCommandTrait.h"


//----------------------------------------------------------------------//
//  UTWOrientationProcessor
//----------------------------------------------------------------------//
UTWOrientationProcessor::UTWOrientationProcessor()
	: EntityQuery(*this)
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server | (int32)EProcessorExecutionFlags::Standalone;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
}

void UTWOrientationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTWAttackFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddConstSharedRequirement<FMassSmoothOrientationParameters>(EMassFragmentPresence::All);
}

void UTWOrientationProcessor::Execute(FMassEntityManager& EntityManager,
                                      FMassExecutionContext& Context)
{
	// Clamp max delta time to avoid force explosion on large time steps (i.e. during initialization).
	const float DeltaTime = FMath::Min(0.1f, Context.GetDeltaTimeSeconds());


	EntityQuery.ForEachEntityChunk(Context, [this, DeltaTime](FMassExecutionContext& Context)
	{
		if (Context.DoesArchetypeHaveTag<FTWMassDeadTag>())
		{
			return;
		}
		
		const FMassSmoothOrientationParameters& OrientationParams = Context.GetConstSharedFragment<
			FMassSmoothOrientationParameters>();

		const TConstArrayView<FMassMoveTargetFragment> MoveTargetList = Context.GetFragmentView<
			FMassMoveTargetFragment>();
		const TConstArrayView<FTWAttackFragment> AttackList = Context.GetFragmentView<FTWAttackFragment>();
		const TArrayView<FTransformFragment> LocationList = Context.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassDesiredMovementFragment> DesiredMovementList = Context.GetMutableFragmentView<
			FMassDesiredMovementFragment>();
		if (Context.DoesArchetypeHaveTag<FTWMassAttackingTag>())
		{
			
			for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
			{
				const FTWAttackFragment& AttackFragment = AttackList[EntityIt];
				const FMassMoveTargetFragment& MoveTarget = MoveTargetList[EntityIt];
				
				const FMassEntityHandle Entity = Context.GetEntity(EntityIt);
				const FMassEntityHandle TargetEntity = AttackList[EntityIt].TargetEntity;
				ATWBaseBuilding* TargetBuilding = AttackList[EntityIt].TargetBuilding.Get();
				// Do not touch transform at all when animating
				if (MoveTarget.GetCurrentAction() == EMassMovementAction::Animate)
				{
					continue;
				}
				bool bIsEntityValid = Context.GetEntityManagerChecked().IsEntityValid(TargetEntity);
				bool bIsBuildingValid = AttackList[EntityIt].TargetBuilding.IsValid();
				FVector CurrentLocation = LocationList[EntityIt].GetTransform().GetLocation();
				FVector TargetLocation = FVector::ZeroVector;
				if (!bIsEntityValid && !bIsBuildingValid)
				{
					continue;
				}
				if (bIsEntityValid)
				{
					TargetLocation = Context.GetEntityManagerChecked().GetFragmentDataPtr<FTransformFragment>(TargetEntity)->GetTransform().GetLocation();
				}else if (bIsBuildingValid)
				{
					TargetLocation = TargetBuilding->GetTransform().GetLocation();
				}
				
				FMassDesiredMovementFragment& DesiredMovement = DesiredMovementList[EntityIt];
				FTransform& CurrentTransform = LocationList[EntityIt].GetMutableTransform();

				const FVector::FReal DesiredHeading = UE::MassNavigation::GetYawFromDirection(TargetLocation - CurrentLocation);
				DesiredMovement.DesiredFacing = FQuat(FVector::UpVector, DesiredHeading);
				
				FQuat Rotation(FVector::UpVector, DesiredHeading);
				CurrentTransform.SetRotation(Rotation);
			}
			
		}
		else
		{
			for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
			{
				const FMassMoveTargetFragment& MoveTarget = MoveTargetList[EntityIt];


				// Do not touch transform at all when animating
				if (MoveTarget.GetCurrentAction() == EMassMovementAction::Animate)
				{
					continue;
				}

				FMassDesiredMovementFragment& DesiredMovement = DesiredMovementList[EntityIt];
				FTransform& CurrentTransform = LocationList[EntityIt].GetMutableTransform();
				const FVector CurrentForward = CurrentTransform.GetRotation().GetForwardVector();
				const FVector::FReal CurrentHeading = UE::MassNavigation::GetYawFromDirection(CurrentForward);

				const float EndOfPathAnticipationDistance = OrientationParams.EndOfPathDuration * MoveTarget.
					DesiredSpeed.
					Get();

				FVector::FReal MoveTargetWeight = 0.5;
				FVector::FReal VelocityWeight = 0.5;

				if (MoveTarget.GetCurrentAction() == EMassMovementAction::Move)
				{
					if (MoveTarget.IntentAtGoal == EMassMovementAction::Stand && MoveTarget.DistanceToGoal <
						EndOfPathAnticipationDistance)
					{
						// Fade towards the movement target direction at the end of the path.
						const float Fade = FMath::Square(
							FMath::Clamp(MoveTarget.DistanceToGoal / EndOfPathAnticipationDistance, 0.0f, 1.0f));
						// zero at end of the path

						MoveTargetWeight = FMath::Lerp(OrientationParams.Standing.MoveTargetWeight,
						                               OrientationParams.Moving.MoveTargetWeight, Fade);
						VelocityWeight = FMath::Lerp(OrientationParams.Standing.VelocityWeight,
						                             OrientationParams.Moving.VelocityWeight, Fade);
					}
					else
					{
						MoveTargetWeight = OrientationParams.Moving.MoveTargetWeight;
						VelocityWeight = OrientationParams.Moving.VelocityWeight;
					}
				}
				else // Stand
				{
					MoveTargetWeight = OrientationParams.Standing.MoveTargetWeight;
					VelocityWeight = OrientationParams.Standing.VelocityWeight;
				}

				const FVector::FReal VelocityHeading = UE::MassNavigation::GetYawFromDirection(
					DesiredMovement.DesiredVelocity);
				const FVector::FReal MovementHeading = UE::MassNavigation::GetYawFromDirection(MoveTarget.Forward);

				const FVector::FReal Ratio = MoveTargetWeight / (MoveTargetWeight + VelocityWeight);
				const FVector::FReal DesiredHeading =
					UE::MassNavigation::LerpAngle(VelocityHeading, MovementHeading, Ratio);
				DesiredMovement.DesiredFacing = FQuat(FVector::UpVector, DesiredHeading);

				const FVector::FReal NewHeading = UE::MassNavigation::ExponentialSmoothingAngle(
					CurrentHeading, DesiredHeading, DeltaTime, OrientationParams.OrientationSmoothingTime);

				FQuat Rotation(FVector::UpVector, NewHeading);
				CurrentTransform.SetRotation(Rotation);
			}
		}
	});
}
