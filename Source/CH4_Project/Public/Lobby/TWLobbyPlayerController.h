// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWLobbyPlayerController.generated.h"

class UTWLobby_Layout;

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
	
	void HandleLeaveLobby();
	
	UPROPERTY()
	class UTWLobby_Layout* LobbyWidgetInstance;
	
	void CreateLobbyWidget();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ASUIPLayerController, Meta = (AllowPrivateAccess))
	TSubclassOf<UUserWidget> UIWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lobby")
	TSubclassOf<class UTWLobby_Layout> LobbyWidgetClass;
};
