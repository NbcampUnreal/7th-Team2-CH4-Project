// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWTitle_Layout.generated.h"

class UButton;
class UEditableText;

UCLASS()
class CH4_PROJECT_API UTWTitle_Layout : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UTWTitle_Layout(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void OnStartButtonClicked();
	
	UFUNCTION()
	void OnHostButtonClicked();
	
	UFUNCTION()
	void OnExitButtonClicked();
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ExitButton;
	
};
