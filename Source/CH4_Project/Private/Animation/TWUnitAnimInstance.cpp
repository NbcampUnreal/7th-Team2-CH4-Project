// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/TWUnitAnimInstance.h"
#include "Mass/TWUnit.h"
#include "Subsystems/TWSoundManagerSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassAgentComponent.h"
#include "MassMovementFragments.h"
#include "Evaluation/MovieSceneEvaluationCustomVersion.h"
#include "Mass/Fragments/TWClientVelocityFragment.h"

void UTWUnitAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (GetOwningActor()->GetNetMode()==ENetMode::NM_DedicatedServer)
	{
		return;
	}
	
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
			
			if (GetOwningActor()->GetNetMode()==ENetMode::NM_Client)
			{
				if (const FTWClientVelocityFragment* VelocityFrag = EntityManager.GetFragmentDataPtr<FTWClientVelocityFragment>(EntityHandle))
				{
					GroundSpeed = VelocityFrag->Velocity.Size2D();
				}				
			}else
			{
				if (const FMassVelocityFragment* VelocityFrag = EntityManager.GetFragmentDataPtr<FMassVelocityFragment>(EntityHandle))
				{
					GroundSpeed = VelocityFrag->Value.Size2D();
				}
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
	bIdDead = false;
}

void UTWUnitAnimInstance::AnimNotify_PlayMeleeAttackSound()
{
	AActor* OwnerActor = GetOwningActor();
	
	if (ATWUnit* Unit = Cast<ATWUnit>(OwnerActor))
	{
		if (UTWSoundManagerSubsystem* SoundManager = GetWorld()->GetGameInstance()->GetSubsystem<UTWSoundManagerSubsystem>())
		{
			FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("SFX.Unit.Attack.Melee"));
			
			SoundManager->PlaySoundByTag(AttackTag, Unit->GetActorLocation(), Unit);
		}
	}
}

void UTWUnitAnimInstance::AnimNotify_PlayRangeAttackSound()
{
	AActor* OwnerActor = GetOwningActor();
	
	if (ATWUnit* Unit = Cast<ATWUnit>(OwnerActor))
	{
		if (UTWSoundManagerSubsystem* SoundManager = GetWorld()->GetGameInstance()->GetSubsystem<UTWSoundManagerSubsystem>())
		{
			FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("SFX.Unit.Attack.Range"));
			
			SoundManager->PlaySoundByTag(AttackTag, Unit->GetActorLocation(), Unit);
		}
	}
}

void UTWUnitAnimInstance::AnimNotify_PlayDeadSound()
{
	AActor* OwnerActor = GetOwningActor();
	
	if (ATWUnit* Unit = Cast<ATWUnit>(OwnerActor))
	{
		if (UTWSoundManagerSubsystem* SoundManager = GetWorld()->GetGameInstance()->GetSubsystem<UTWSoundManagerSubsystem>())
		{
			FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("SFX.Unit.Dead"));
			
			SoundManager->PlaySoundByTag(DeadTag, Unit->GetActorLocation(), Unit);
		}
	}
}
