// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobby_Layout.h"
#include "Lobby//TWLobbyPlayerController.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/HorizontalBox.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Title/TWTitlePlayerController.h"
#include "GameFramework/GameStateBase.h"

UTWLobby_Layout::UTWLobby_Layout(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UTWLobby_Layout::NativeConstruct()
{
	Super::NativeConstruct();
	
	PlayButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnPlayButtonClicked);
	ReadyButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnReadyButtonClicked);
	CancelButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnCancelButtonClicked);
	LobbyExitButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnLobbyExitButtonClicked);
	
	NickNameSlots.Empty();
	NickNameSlots.Add(NickName_1);
	NickNameSlots.Add(NickName_2);
	NickNameSlots.Add(NickName_3);
	NickNameSlots.Add(NickName_4);
}

void UTWLobby_Layout::OnPlayButtonClicked()
{
	ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if(PC)
	{
		PC->Server_RequestStartGame();
	}
}

void UTWLobby_Layout::OnReadyButtonClicked()
{
	ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->Server_SetReady(true);
	}
}

void UTWLobby_Layout::OnCancelButtonClicked()
{
	ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->Server_SetReady(false);
	}
}

void UTWLobby_Layout::OnLobbyExitButtonClicked()
{
	ATWLobbyPlayerController* PC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (PC)
	{
		PC->ExitLobby();
	}
}

void UTWLobby_Layout::UpdateUserList()
{
	for (UHorizontalBox* NickSlot : NickNameSlots)
	{
		if (NickSlot)
		{
			NickSlot->SetVisibility(ESlateVisibility::Collapsed);
		}
		
		AGameStateBase* GS = GetWorld()->GetGameState();
		
		if (GS)
		{
			for (int32 i = 0; i< GS->PlayerArray.Num(); i++)
			{
				if (NickNameSlots.IsValidIndex(i) && NickNameSlots[i])
				{
					NickNameSlots[i]->SetVisibility(ESlateVisibility::Visible);
				}
			}
		}
	}
}

void UTWLobby_Layout::ShowPlayButton(bool bIsShow)
{
	if (bIsShow)
	{
		PlayButton->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		PlayButton->SetVisibility(ESlateVisibility::Hidden);
	}
}
