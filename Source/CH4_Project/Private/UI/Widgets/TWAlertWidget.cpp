// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/TWAlertWidget.h"

#include "Components/TextBlock.h"

void UTWAlertWidget::SetAlertText(const FString& Message)
{
	if (AlertTextBlock)
	{
		AlertTextBlock->SetText(FText::FromString(Message));
	}
}
