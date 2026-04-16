// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TWLobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATWLobbyGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ATWLobbyGameMode();
	
	// 우리가 앞서 만든 JoinServer("IP?Name=NickName")의 Name 데이터를 실제로 꺼내서 PlayerState에 넣어줄 최적의 장소이다.
	// BeginPlay 보다 더 확실하게 플레이어가 접속된 시점이다.
	virtual void PreLogin(
		const FString& Options, 
		const FString& Address, 
		const FUniqueNetIdRepl& UniqueId, 
		FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	// 이 함수는 PlayerController가 서버에 "준비 완료"라고 신호를 보낼 때마다 호출될 것이다.
	// 서버는 이 함수를 통해 "게임 시작 버튼의 활성화 여부"를 판단한다.
	void CheckStartCondition();
	void StartGame();
	
protected:
	void AssignNewHost();
	
private:
	int32 MinPlayersToStart = 2;
};


