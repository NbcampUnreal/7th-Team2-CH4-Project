// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TESTPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATESTPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void OnInputClick(); 
	
	UFUNCTION(Server, BlueprintCallable, Reliable)
	void ServerMoveCommand(FVector Location);
};
