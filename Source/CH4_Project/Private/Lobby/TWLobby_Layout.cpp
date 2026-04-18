// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/TWLobby_Layout.h"
#include "Lobby//TWLobbyPlayerController.h"
#include "Lobby/TWLobbyGameState.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/HorizontalBox.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Title/TWTitlePlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Lobby/TWLobbyGameMode.h"

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
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	ATWLobbyPlayerState* LPS = LPC->GetPlayerState<ATWLobbyPlayerState>();
	if (!LPC || !LPS->IsHost()) return;
	
	if(LPC)
	{
		LPC->Server_RequestStartGame();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can Not Start!!!"));
	}
}

void UTWLobby_Layout::OnReadyButtonClicked()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (LPC)
	{
		LPC->Server_SetReady(true);
	}
}

void UTWLobby_Layout::OnCancelButtonClicked()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (LPC)
	{
		LPC->Server_SetReady(false);
	}
}

void UTWLobby_Layout::OnLobbyExitButtonClicked()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (LPC)
	{
		LPC->ExitLobby();
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
		
		ATWLobbyGameState* LGS = Cast<ATWLobbyGameState>(GetWorld()->GetGameState());
		
		if (LGS)
		{
			for (int32 i = 0; i< LGS->PlayerArray.Num(); i++)
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
