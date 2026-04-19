// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWSystemMenuWidget.generated.h"

class UButton;

UCLASS()
class CH4_PROJECT_API UTWSystemMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UTWSystemMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void OnExitButtonClicked();

	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USysteMenuWidget, meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> ExitButton;
};

