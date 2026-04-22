// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/TWDefeatWidget.h"
#include "Components/Button.h"

UTWDefeatWidget::UTWDefeatWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UTWDefeatWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ExitButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnExitButtonClicked);
}

void UTWDefeatWidget::OnExitButtonClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		PC->ClientTravel(TEXT("L_Title"), TRAVEL_Absolute);
	}
}
