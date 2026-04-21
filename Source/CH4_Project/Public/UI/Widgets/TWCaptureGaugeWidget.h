// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWCaptureGaugeWidget.generated.h"


class UProgressBar;
class UTextBlock;

UCLASS()
class CH4_PROJECT_API UTWCaptureGaugeWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateGauge(float Current, float Max, int32 TeamID);
	
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> CaptureProgressBar;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ProgressText;
};
