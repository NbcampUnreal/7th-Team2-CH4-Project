// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Loading/TWLoadingPlayerController.h"

#include "Blueprint/UserWidget.h"

void ATWLoadingPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (LoadingWidgetClass)
	{
		UUserWidget* LoadingWidget = CreateWidget<UUserWidget>(this, LoadingWidgetClass);
		if (LoadingWidget)
		{
			LoadingWidget->AddToViewport();
		}
	}
}
