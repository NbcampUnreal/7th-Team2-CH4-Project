// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "GameFramework/PlayerController.h"
#include "TWPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATWPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	ATWPlayerController();
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
#pragma region Input
protected:
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SelectAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveCommandAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackCommandAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HoldCommandAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScreenEdgeMargin = 10.0f; 
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScrollSpeed = 1500.0f; 
	UFUNCTION()
	void OnStartSelectAction(const FInputActionValue& InputActionValue);
	void OnEndSelectAction(const FInputActionValue& InputActionValue);
	void OnMoveCommandAction(const FInputActionValue& InputActionValue);
	void OnAttackCommandAction(const FInputActionValue& InputActionValue);
	void OnHoldCommandAction(const FInputActionValue& InputActionValue);
	
	UFUNCTION(Server,Reliable)
	void ServerHandleMoveCommand(const FVector& CommandLocation);
	UFUNCTION(Server,Reliable)
	void ServerHandleSelect(const FVector& CommandLocation);
	
	
private:
	void HandleScreenEdgeScrolling(float DeltaSeconds);
#pragma endregion
	
private:
	TArray<FMassEntityHandle> SelectedEntities;
	
};
