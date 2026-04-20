// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TWMassBubbleTickGuardSubsystem.generated.h"

/**
 * Seamless travel 직전 현재 월드의 AMassClientBubbleInfoBase 상속 actor들의
 * Tick을 꺼서, 월드 teardown 중 MassClientBubbleHandler::Tick 내부
 * check(World) assert가 터지는 것을 방지한다.
 */
UCLASS()
class CH4_PROJECT_API UTWMassBubbleTickGuardSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnWorldBeginTearDown(UWorld* World);

	FDelegateHandle WorldBeginTearDownHandle;
};
