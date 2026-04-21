// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TWTitle_Layout.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Title/TWTitlePlayerController.h"

UTWTitle_Layout::UTWTitle_Layout(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UTWTitle_Layout::NativeConstruct()
{
	StartButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnStartButtonClicked);
	HostButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnHostButtonClicked);
	ExitButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnExitButtonClicked);
}

void UTWTitle_Layout::OnStartButtonClicked()
{
	ATWTitlePlayerController* PC = GetOwningPlayer<ATWTitlePlayerController>();
	
	if (IsValid(PC) == true)
	{
		PC->JoinServer();
	}
}

void UTWTitle_Layout::OnHostButtonClicked()
{
	UGameplayStatics::OpenLevel(GetWorld(), FName("L_Lobby"), true, "listen");
}

void UTWTitle_Layout::OnExitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
