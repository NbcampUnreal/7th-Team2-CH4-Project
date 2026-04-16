// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TWTitleGameMode.h"

void ATWTitleGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	APlayerController* NewPlayerController = Cast<APlayerController>(NewPlayer);
	if (IsValid(NewPlayerController) == true)
	{
		AlivePlayerControllers.Add(NewPlayerController);
	}
}

void ATWTitleGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	
	APlayerController* ExitController = Cast<APlayerController>(Exiting);
	if (IsValid(ExitController) == true)
	{
		AlivePlayerControllers.Remove(ExitController);
		// 클라이언트가 강제 종료될 때의 예외처리 해야 함
		DeadPlayerControllers.Add(ExitController);
	}
}
