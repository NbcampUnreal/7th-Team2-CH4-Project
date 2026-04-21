// Fill out your copyright notice in the Description page of Project Settings.


#include "TWCheatManager.h"

#include "Core/TWPlayerController.h"
#include "Data/TWBuildingTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

void UTWCheatManager::AddResource(FString ResourceType, int32 Amount)
{
	EResourceType TargetResourceType = EResourceType::None;
	
	if (ResourceType.Equals(TEXT("Wood"), ESearchCase::IgnoreCase))
	{
		TargetResourceType = EResourceType::Wood;
	}
	else if (ResourceType.Equals(TEXT("Ore"), ESearchCase::IgnoreCase))
	{
		TargetResourceType = EResourceType::Ore;
	}
	else if (ResourceType.Equals(TEXT("Mithril"), ESearchCase::IgnoreCase))
	{
		TargetResourceType = EResourceType::Mithril;
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Cheat 실패: Wood, Ore, Mithril 만 입력 가능"));
		}
		return;
	}
	
	if (ATWPlayerController* PC = Cast<ATWPlayerController>(GetOuterAPlayerController()))
	{
		PC->Server_CheatAddResource(TargetResourceType, Amount);
		if (GEngine)
		{
			FString Msg = FString::Printf(TEXT("[Cheat/Client] 서버에 %s 자원 %d 추가를 요청했습니다."), *ResourceType, Amount);
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, Msg);
		}
	}
}

void UTWCheatManager::TimeScale(float TimeMultiplier)
{
	if (ATWPlayerController* PC = Cast<ATWPlayerController>(GetOuterAPlayerController()))
	{
		PC->Server_CheatTimeScale(TimeMultiplier);
	}
}
