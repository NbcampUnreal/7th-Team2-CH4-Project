// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TWTitlePlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Log/TWLogCategory.h"

void ATWTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController() == false)
	{
		return;
	}
	
	if (IsValid(UIWidgetClass))
	{
		UIWidgetInstance = CreateWidget<UUserWidget>(this, UIWidgetClass);
		if (IsValid(UIWidgetInstance))
		{
			UIWidgetInstance->AddToViewport();
			FInputModeUIOnly Mode;
			SetInputMode(Mode);
			bShowMouseCursor = true;
		}
	}
	/*if (GetWorld()->GetNetMode() == NM_Standalone)
	{
		UE_LOG(LogTemp, Warning, TEXT("NM_Standalone"));
	}
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("NM_Client!!"));
	}
	if (GetWorld()->GetNetMode() == NM_ListenServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("NM_ListenServer!!"));
	}
	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("NM_DedicatedServer!!"));
	}*/
}

void ATWTitlePlayerController::JoinServer()
{
	/*UGameplayStatics::OpenLevel(GetWorld(), "L_Lobby");*/
	UE_LOG(LogTWTitle, Error, TEXT("=== [DEBUG] JoinServer Start ==="));
	const FString URL = "127.0.0.1:7777";
	ClientTravel(URL, TRAVEL_Absolute);
	UE_LOG(LogTWTitle, Error, TEXT("=== [DEBUG] ClientTravel Called with URL: %s ==="), *URL);
	/*ClientTravel("L_LobbyLevel", TRAVEL_Absolute);*/
}
