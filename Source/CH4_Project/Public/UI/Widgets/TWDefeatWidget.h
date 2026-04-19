// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWDefeatWidget.generated.h"

class UButton;

UCLASS()
class CH4_PROJECT_API UTWDefeatWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UTWDefeatWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void OnExitButtonClicked();
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = UVictoryWidget, meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> ExitButton;
};
