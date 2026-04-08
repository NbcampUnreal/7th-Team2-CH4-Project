// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "GameFramework/PlayerController.h"
#include "Mass/Fragments/CommandFragment.h"
#include "TWPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UENUM()
enum class ETWCommand:uint8
{
	None,
	Move,
	Attack,
	Hold
};


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
	TObjectPtr<UInputAction> LeftMouseAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> RightMouseAction;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveCommandAction;//m
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackCommandAction;//a
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HoldCommandAction;//h
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScreenEdgeMargin = 10.0f; 
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float ScrollSpeed = 1500.0f; 
	UFUNCTION()
	void OnStartLeftMouseAction(const FInputActionValue& InputActionValue);
	void OnEndLeftMouseAction(const FInputActionValue& InputActionValue);
	void OnRightMouseAction(const FInputActionValue& InputActionValue);//명령 있을 시 취소, 없으면 이동명령
	void OnMoveCommandAction(const FInputActionValue& InputActionValue);
	void OnAttackCommandAction(const FInputActionValue& InputActionValue);
	void OnHoldCommandAction(const FInputActionValue& InputActionValue);
	
	UFUNCTION(Server,Reliable)
	void ServerHandleMoveCommand(const FVector& CommandLocation);
	UFUNCTION(Server,Reliable)
	void ServerHandleAttackCommand(const FVector& CommandLocation);
	UFUNCTION(Server,Reliable)
	void ServerHandleHoldCommand();
	UFUNCTION(Server,Reliable)
	void ServerHandleSelect(const FVector& CommandLocation);
	
	
private:
	void HandleScreenEdgeScrolling(float DeltaSeconds);
#pragma endregion
	
private:
	void ChangeCurrentCommandType(ETWCommand CommandType);
private:
	ETWCommand CurrentCommandType;
	TArray<FMassEntityHandle> SelectedEntities;
	FVector ClickStartLocation;
	
};
