// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWAlertWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class CH4_PROJECT_API UTWAlertWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetAlertText(const FString& Message);
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AlertTextBlock;
};
