// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TWLobbyPlayerState.generated.h"


UCLASS()
class CH4_PROJECT_API ATWLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
	ATWLobbyPlayerState();
	
public:
	void SetMyNickName(const FString& InNickName);
	void SetIsReady(bool bInReady);
	void SetIsHost(bool bInHost);
	
	// Getter
	FORCEINLINE bool IsReady() const {return bIsReady;}
	FORCEINLINE bool IsHost() const {return bIsHost;}
	FORCEINLINE FString GetMyNickName() const {return MyNickName;}
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsReady;
	
	UPROPERTY(ReplicatedUsing = OnRep_MyNickName)
	FString MyNickName;
	
	UPROPERTY(ReplicatedUsing = OnRep_IsHost)
	bool bIsHost;
	
	UFUNCTION()
	void OnRep_IsReady();
	
	UFUNCTION()
	void OnRep_MyNickName();
	
	UFUNCTION()
	void OnRep_IsHost();
	
};

