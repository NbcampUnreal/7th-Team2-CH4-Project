// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWHeroTestController.generated.h"

class ATWHeroUnitBase;
class UInputAction;
class UInputMappingContext;
/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATWHeroTestController : public APlayerController
{
	GENERATED_BODY()

public:
	ATWHeroTestController();

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	/** Enhanced Input 자산 */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_UseSkill;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Confirm;

	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> IA_Cancel;

	/** 입력 핸들러 */
	void OnSkillAction();
	void OnConfirmAction();
	void OnCancelAction();

private:
	UPROPERTY()
	TObjectPtr<ATWHeroUnitBase> ControlledHero;

	bool bIsTargetingMode = false;

	FVector GetMouseWorldLocation() const;
};
