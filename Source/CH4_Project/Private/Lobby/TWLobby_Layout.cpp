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
#include "Components/Image.h"

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
	
	HostImages.Empty();
	HostImages.Add(HostImage1);
	HostImages.Add(HostImage2);
	HostImages.Add(HostImage3);
	HostImages.Add(HostImage4);
	
	ReadyImages.Empty();
	ReadyImages.Add(ReadyImage1);
	ReadyImages.Add(ReadyImage2);
	ReadyImages.Add(ReadyImage3);
	ReadyImages.Add(ReadyImage4);
	
	if (PlayButton->IsVisible())
	{
		PlayButton->SetVisibility(ESlateVisibility::Hidden);
	}
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

void UTWLobby_Layout::SubmitNickname(const FString& InNickname)
{
	if (ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer()))
	{
		LPC->Server_SetLobbyNickname(InNickname);
	}
}

void UTWLobby_Layout::SelectHero(FName InHeroUnitId)
{
	if (ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer()))
	{
		LPC->Server_SetSelectedHeroUnitId(InHeroUnitId);
	}
}

void UTWLobby_Layout::UpdateUserList()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	ATWLobbyGameState* LGS = Cast<ATWLobbyGameState>(GetWorld()->GetGameState());
	if (!LGS) return;
	
	int32 PlayerCount = LGS->GetCurrentPlayerCount();
	
	for (int32 i = 0; i < NickNameSlots.Num(); i++)
	{
		if (i < PlayerCount)
		{
			NickNameSlots[i]->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			NickNameSlots[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTWLobby_Layout::UpdateUserImage()
{
	AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS) return;
	
	for (int32 i = 0; i < NickNameSlots.Num(); i++)
	{
		if (NickNameSlots[i]->GetVisibility() == ESlateVisibility::Visible)
		{
			if (GS->PlayerArray.IsValidIndex(i))
			{
				ATWLobbyPlayerState* LPS = Cast<ATWLobbyPlayerState>(GS->PlayerArray[i]);
				if (!LPS) continue;
				
				if (HostImages.IsValidIndex(i) && HostImages[i])
				{
					if (LPS->IsHost())
					{
						HostImages[i]->SetVisibility(ESlateVisibility::Visible);
					}
				}
				if (ReadyImages.IsValidIndex(i) && ReadyImages[i])
				{
					if (!LPS->IsHost() && LPS->IsReady())
					{
						ReadyImages[i]->SetVisibility(ESlateVisibility::Visible);
					}
					else
					{
						ReadyImages[i]->SetVisibility(ESlateVisibility::Collapsed);
					}
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
