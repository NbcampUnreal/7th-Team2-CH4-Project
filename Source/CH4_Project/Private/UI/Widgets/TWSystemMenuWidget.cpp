// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/TWSystemMenuWidget.h"
#include "Components/Button.h"

UTWSystemMenuWidget::UTWSystemMenuWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UTWSystemMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ExitButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnExitButtonClicked);
}

void UTWSystemMenuWidget::OnExitButtonClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		PC->ClientTravel(TEXT("L_Title"), TRAVEL_Relative);
	}
}