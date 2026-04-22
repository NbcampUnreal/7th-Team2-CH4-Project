// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TWLobbyPlayerState.generated.h"

UCLASS()
class CH4_PROJECT_API ATWLobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ATWLobbyPlayerState();

	void SetIsReady(bool bInReady);
	void SetIsHost(bool bInHost);
	void SetLobbyNickname(const FString& InNickname);
	void SetSelectedHeroUnitId(FName InHeroUnitId);

	virtual void PostNetInit() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	// Getter
	FORCEINLINE bool IsReady() const { return bIsReady; }
	FORCEINLINE bool IsHost() const { return bIsHost; }
	FORCEINLINE const FString& GetLobbyNickname() const { return LobbyNickname; }
	FORCEINLINE FName GetSelectedHeroUnitId() const { return SelectedHeroUnitId; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_IsReady();

	UFUNCTION()
	void OnRep_IsHost();

	UFUNCTION()
	void OnRep_LobbyNickname();

	UFUNCTION()
	void OnRep_SelectedHeroUnitId();

	void RefreshLobbyWidget() const;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_IsReady)
	bool bIsReady;

	UPROPERTY(ReplicatedUsing = OnRep_IsHost)
	bool bIsHost;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyNickname)
	FString LobbyNickname;

	UPROPERTY(ReplicatedUsing = OnRep_SelectedHeroUnitId)
	FName SelectedHeroUnitId = NAME_None;
};