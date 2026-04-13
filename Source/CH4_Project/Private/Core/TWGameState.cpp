// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/TWGameState.h"

#include "EngineUtils.h"
#include "Core/TWPlayerState.h"

ATWPlayerState* ATWGameState::GetPlayerState(int32 PlayerSlot)
{
	for(TActorIterator<ATWPlayerState> It(GetWorld()); It; ++It){
		if (It->PlayerSlot == PlayerSlot)
		{
			return *It;
		}
	}
	return nullptr;
}
