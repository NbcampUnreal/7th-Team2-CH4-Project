// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityHandle.h"
#include "GameFramework/PlayerController.h"
#include "Mass/Fragments/TWCommandFragment.h"
#include "TWPlayerController.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class ATWPopulationBuilding;
class ATWBlockingBuilding;
class AGhostBuilding;
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
	TObjectPtr<UInputAction> BuildCommandAction;
	
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
	void OnBuildCommandAction(const FInputActionValue& InputActionValue);
	
	UFUNCTION(Server,Reliable)
	void ServerHandleMoveCommand(const FVector& CommandLocation);
	UFUNCTION(Server,Reliable)
	void ServerHandleAttackCommand(const FVector& CommandLocation);
	UFUNCTION(Server,Reliable)
	void ServerHandleHoldCommand();
	UFUNCTION(Server,Reliable)
	void ServerHandleSingleSelect(const FVector& CommandLocation);
	UFUNCTION(Server,Reliable)
	void ServerHandleMultipleSelect(const FVector& StartLocation, const FVector& EndLocation);
	
private:
	void HandleScreenEdgeScrolling(float DeltaSeconds);
#pragma endregion
	
#pragma region 병력 스폰 대기열
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_TestSpawnTroop;

	UFUNCTION()
	void HandleTestSpawnTroop(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestSpawnTroop();
#pragma endregion

#pragma region 인구 수 대기열	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_TestIncreasePopulation;
	
	UFUNCTION()
	void HandleTestIncreasePopulation(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestIncreasePopulation();
#pragma endregion
	
#pragma region 방벽 데미지
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_TestDamageBlockingBuilding;

	UFUNCTION()
	void HandleTestDamageBlockingBuilding(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestDamageBlockingBuilding();
#pragma endregion

#pragma region 건설
	
protected:
	UFUNCTION(Server,Reliable, Category = "Build")
	void Server_SpawnBuilding(FIntPoint Anchor, FIntPoint BuildSize, TSubclassOf<AActor> ClassToSpawn);
	
public:
	
	UFUNCTION(BlueprintCallable, Category = "Build")
	void ToggleBuildMode();
	UFUNCTION(BlueprintCallable, Category = "Build")
	void RequestBuild();

	void EndBuildMode();
	
private:
	uint8 bIsBuildMode : 1;
	FIntPoint CurrentAnchor;
	
	UPROPERTY()
	TObjectPtr<AGhostBuilding> CurrentGhost;
	
	UPROPERTY(EditAnywhere, Category = "Build|Classes")
	TSubclassOf<AGhostBuilding> BuildClass;
	UPROPERTY(EditAnywhere, Category = "Build|Classes")
	TSubclassOf<AActor> SelectedBuildingClass;
	
#pragma endregion
	
private:
	void ChangeCurrentCommandType(ETWCommand CommandType);
private:
	ETWCommand CurrentCommandType;
	TArray<FMassEntityHandle> SelectedEntities;
	FVector ClickStartLocation;
	
	
	UFUNCTION(Server, Reliable)
	void TESTSPAWNCODE();
protected:
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMassEntityConfigAsset> TestMassEntityConfigAsset;
};
