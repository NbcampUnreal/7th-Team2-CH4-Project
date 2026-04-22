// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWLobbyPlayerController.generated.h"

class UTWLobby_Layout;
class UUserWidget;

UCLASS()
class CH4_PROJECT_API ATWLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetReady(bool bNewReady);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestStartGame();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetLobbyNickname(const FString& InNickname);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetSelectedHeroUnitId(FName InHeroUnitId);

	void ExitLobby();

	UPROPERTY()
	TObjectPtr<UTWLobby_Layout> LobbyWidgetInstance;

	void CreateLobbyWidget();
	void RefreshLobbyWidget();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ATWUIPLayerController)
	TSubclassOf<UUserWidget> UIWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
	TSubclassOf<UTWLobby_Layout> LobbyWidgetClass;
};