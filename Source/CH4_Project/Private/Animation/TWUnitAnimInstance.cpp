// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/TWUnitAnimInstance.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassAgentComponent.h"
#include "MassMovementFragments.h"
#include "Evaluation/MovieSceneEvaluationCustomVersion.h"
#include "Mass/Fragments/TWClientVelocityFragment.h"

void UTWUnitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!MassSubsystem || !MassAgentComponent)
	{
		GroundSpeed = 0.0f;
		return;
	}
	
	FMassEntityHandle EntityHandle = MassAgentComponent->GetEntityHandle();
	if (EntityHandle.IsValid())
	{
		FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();
		
		if (EntityManager.IsEntityActive(EntityHandle))
		{
			if (GetOwningActor()->GetNetMode()==NM_ListenServer)
			{
				if (const FMassVelocityFragment* VelocityFrag = EntityManager.GetFragmentDataPtr<FMassVelocityFragment>(EntityHandle))
				{
					GroundSpeed = VelocityFrag->Value.Size2D();
				}
			}else if (GetOwningActor()->GetNetMode()==ENetMode::NM_Client)
			{
				if (const FTWClientVelocityFragment* VelocityFrag = EntityManager.GetFragmentDataPtr<FTWClientVelocityFragment>(EntityHandle))
				{
					GroundSpeed = VelocityFrag->Velocity.Size2D();
				}				
			}
			
			if (GroundSpeed<10)
			{
				UE_LOG(LogTemp,Warning,TEXT("GroundSpeed<10"))
			}
			return;
			
		}
	}
	GroundSpeed = 0.0f;
}

void UTWUnitAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	if (UWorld* World = GetWorld())
	{
		MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	}
	
	if (AActor* OwningActor = GetOwningActor())
	{
		MassAgentComponent = OwningActor->FindComponentByClass<UMassAgentComponent>();
	}
}
