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
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;
	
private:
	UPROPERTY()
	TObjectPtr<UMassEntitySubsystem> MassSubsystem;
	
	UPROPERTY()
	TObjectPtr<UMassAgentComponent> MassAgentComponent;
};
