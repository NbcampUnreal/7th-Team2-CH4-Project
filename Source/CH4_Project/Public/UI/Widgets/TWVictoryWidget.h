// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWVictoryWidget.generated.h"

class UButton;

UCLASS()
class CH4_PROJECT_API UTWVictoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UTWVictoryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void OnExitButtonClicked();
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = UVictoryWidget, meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> ExitButton;
};
