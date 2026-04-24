// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TWUnitAnimInstance.generated.h"

class UMassAgentComponent;
class UMassEntitySubsystem;
class APawn;
class UPawnMovementComponent;

UCLASS()
class CH4_PROJECT_API UTWUnitAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeInitializeAnimation() override;
	
	UFUNCTION()
	void SetIsDead(){bIdDead = true;}
	
	UFUNCTION()
	void AnimNotify_PlayMeleeAttackSound();
	
	UFUNCTION()
	void AnimNotify_PlayRangeAttackSound();
	
	UFUNCTION()
	void AnimNotify_PlayDeadSound();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bIdDead = false;
private:
	UPROPERTY()
	TObjectPtr<UMassEntitySubsystem> MassSubsystem;
	
	UPROPERTY()
	TObjectPtr<UMassAgentComponent> MassAgentComponent;
};
