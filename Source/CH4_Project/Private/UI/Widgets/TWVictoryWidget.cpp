// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/TWVictoryWidget.h"
#include "Components/Button.h"

UTWVictoryWidget::UTWVictoryWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UTWVictoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ExitButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnExitButtonClicked);
}

void UTWVictoryWidget::OnExitButtonClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		PC->ClientTravel(TEXT("L_Title"), TRAVEL_Absolute);
	}
}
