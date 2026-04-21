// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/TWSystemMenuWidget.h"
#include "Components/Button.h"
#include "Core/TWPlayerController.h"

UTWSystemMenuWidget::UTWSystemMenuWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

void UTWSystemMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ExitButton->OnClicked.RemoveAll(this);
	ExitButton->OnClicked.AddDynamic(this, &ThisClass::OnExitButtonClicked);
}

void UTWSystemMenuWidget::OnExitButtonClicked()
{
	ATWPlayerController* PC = Cast<ATWPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->Server_RequestDefeat();
		PC->ClientTravel(TEXT("L_Title"), TRAVEL_Absolute);
	}
}