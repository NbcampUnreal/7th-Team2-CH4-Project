// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby/TWLobby_Layout.h"

#include "Lobby/TWLobbyPlayerController.h"
#include "Lobby/TWLobbyGameState.h"
#include "Lobby/TWLobbyPlayerState.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/ComboBoxString.h"
#include "CommonTextBlock.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "TimerManager.h"

UTWLobby_Layout::UTWLobby_Layout(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTWLobby_Layout::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &ThisClass::OnPlayButtonClicked);
	}

	if (ReadyButton)
	{
		ReadyButton->OnClicked.AddDynamic(this, &ThisClass::OnReadyButtonClicked);
	}

	if (CancelButton)
	{
		CancelButton->OnClicked.AddDynamic(this, &ThisClass::OnCancelButtonClicked);
	}

	if (LobbyExitButton)
	{
		LobbyExitButton->OnClicked.AddDynamic(this, &ThisClass::OnLobbyExitButtonClicked);
	}

	if (ApplySettingsButton)
	{
		ApplySettingsButton->OnClicked.AddDynamic(this, &ThisClass::OnApplySettingsButtonClicked);
	}

	if (HeroSelectComboBox)
	{
		HeroSelectComboBox->ClearOptions();
		HeroSelectComboBox->AddOption(TEXT("DragonKnight"));
		HeroSelectComboBox->AddOption(TEXT("Markman"));
		HeroSelectComboBox->AddOption(TEXT("Astrologian"));

		if (HeroSelectComboBox->GetSelectedOption().IsEmpty())
		{
			HeroSelectComboBox->SetSelectedOption(TEXT("DragonKnight"));
		}
	}
	
	NickNameSlots.Empty();
	NickNameSlots.Add(NickName_1);
	NickNameSlots.Add(NickName_2);
	NickNameSlots.Add(NickName_3);
	NickNameSlots.Add(NickName_4);

	PlayerNameTexts.Empty();
	PlayerNameTexts.Add(PlayerName1);
	PlayerNameTexts.Add(PlayerName2);
	PlayerNameTexts.Add(PlayerName3);
	PlayerNameTexts.Add(PlayerName4);

	HeroTexts.Empty();
	HeroTexts.Add(HeroText1);
	HeroTexts.Add(HeroText2);
	HeroTexts.Add(HeroText3);
	HeroTexts.Add(HeroText4);
	
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
	
	if (PlayButton && PlayButton->IsVisible())
	{
		PlayButton->SetVisibility(ESlateVisibility::Hidden);
	}

	UpdateUserList();
}

void UTWLobby_Layout::OnPlayButtonClicked()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (!LPC)
	{
		return;
	}

	ATWLobbyPlayerState* LPS = LPC->GetPlayerState<ATWLobbyPlayerState>();
	if (!LPS || !LPS->IsHost())
	{
		return;
	}
	
	LPC->Server_RequestStartGame();
}

void UTWLobby_Layout::OnReadyButtonClicked()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (LPC)
	{
		LPC->Server_SetReady(true);
	}

	UpdateUserList();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredRefreshTimerHandle);
		World->GetTimerManager().SetTimer(
			DeferredRefreshTimerHandle,
			this,
			&ThisClass::DeferredRefreshUserList,
			0.15f,
			false
		);
	}
}

void UTWLobby_Layout::OnCancelButtonClicked()
{
	ATWLobbyPlayerController* LPC = Cast<ATWLobbyPlayerController>(GetOwningPlayer());
	if (LPC)
	{
		LPC->Server_SetReady(false);
	}

	UpdateUserList();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredRefreshTimerHandle);
		World->GetTimerManager().SetTimer(
			DeferredRefreshTimerHandle,
			this,
			&ThisClass::DeferredRefreshUserList,
			0.15f,
			false
		);
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

void UTWLobby_Layout::OnApplySettingsButtonClicked()
{
	if (NickNameInput)
	{
		const FString InputNickname = NickNameInput->GetText().ToString().TrimStartAndEnd();
		if (!InputNickname.IsEmpty())
		{
			SubmitNickname(InputNickname);
		}
	}

	if (HeroSelectComboBox)
	{
		const FString SelectedHero = HeroSelectComboBox->GetSelectedOption();
		if (!SelectedHero.IsEmpty())
		{
			SelectHero(FName(*SelectedHero));
		}
	}

	UpdateUserList();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredRefreshTimerHandle);
		World->GetTimerManager().SetTimer(
			DeferredRefreshTimerHandle,
			this,
			&ThisClass::DeferredRefreshUserList,
			0.15f,
			false
		);
	}
}

void UTWLobby_Layout::DeferredRefreshUserList()
{
	UpdateUserList();
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

FString UTWLobby_Layout::GetDisplayHeroName(const FName& InHeroUnitId) const
{
	if (InHeroUnitId.IsNone())
	{
		return TEXT("-");
	}

	if (InHeroUnitId == TEXT("DragonKnight"))
	{
		return TEXT("DragonKnight");
	}

	if (InHeroUnitId == TEXT("Markman"))
	{
		return TEXT("Markman");
	}

	if (InHeroUnitId == TEXT("Astrologian"))
	{
		return TEXT("Astrologian");
	}

	return InHeroUnitId.ToString();
}

void UTWLobby_Layout::RefreshUserSlot(int32 SlotIndex)
{
	ATWLobbyGameState* LGS = Cast<ATWLobbyGameState>(GetWorld() ? GetWorld()->GetGameState() : nullptr);
	if (!LGS)
	{
		return;
	}

	if (!NickNameSlots.IsValidIndex(SlotIndex) || !NickNameSlots[SlotIndex])
	{
		return;
	}

	const int32 PlayerCount = LGS->GetCurrentPlayerCount();
	const bool bVisible = SlotIndex < PlayerCount;

	NickNameSlots[SlotIndex]->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	if (!bVisible)
	{
		if (PlayerNameTexts.IsValidIndex(SlotIndex) && PlayerNameTexts[SlotIndex])
		{
			PlayerNameTexts[SlotIndex]->SetText(FText::GetEmpty());
		}

		if (HeroTexts.IsValidIndex(SlotIndex) && HeroTexts[SlotIndex])
		{
			HeroTexts[SlotIndex]->SetText(FText::GetEmpty());
		}

		if (HostImages.IsValidIndex(SlotIndex) && HostImages[SlotIndex])
		{
			HostImages[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (ReadyImages.IsValidIndex(SlotIndex) && ReadyImages[SlotIndex])
		{
			ReadyImages[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}

		return;
	}

	if (!LGS->PlayerArray.IsValidIndex(SlotIndex))
	{
		return;
	}

	ATWLobbyPlayerState* LPS = Cast<ATWLobbyPlayerState>(LGS->PlayerArray[SlotIndex]);
	if (!LPS)
	{
		return;
	}

	const FString LobbyNickname = LPS->GetLobbyNickname();
	const FName SelectedHeroUnitId = LPS->GetSelectedHeroUnitId();

	if (PlayerNameTexts.IsValidIndex(SlotIndex) && PlayerNameTexts[SlotIndex])
	{
		const FString DisplayName = LobbyNickname.IsEmpty()
			? FString::Printf(TEXT("Player%d"), SlotIndex + 1)
			: LobbyNickname;

		PlayerNameTexts[SlotIndex]->SetText(FText::FromString(DisplayName));
	}

	if (HeroTexts.IsValidIndex(SlotIndex) && HeroTexts[SlotIndex])
	{
		HeroTexts[SlotIndex]->SetText(FText::FromString(GetDisplayHeroName(SelectedHeroUnitId)));
	}

	if (HostImages.IsValidIndex(SlotIndex) && HostImages[SlotIndex])
	{
		HostImages[SlotIndex]->SetVisibility(
			LPS->IsHost() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed
		);
	}

	if (ReadyImages.IsValidIndex(SlotIndex) && ReadyImages[SlotIndex])
	{
		if (!LPS->IsHost() && LPS->IsReady())
		{
			ReadyImages[SlotIndex]->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			ReadyImages[SlotIndex]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UTWLobby_Layout::UpdateUserList()
{
	ATWLobbyGameState* LGS = Cast<ATWLobbyGameState>(GetWorld() ? GetWorld()->GetGameState() : nullptr);
	if (!LGS)
	{
		return;
	}
	
	for (int32 i = 0; i < NickNameSlots.Num(); i++)
	{
		RefreshUserSlot(i);
	}
}

void UTWLobby_Layout::UpdateUserImage()
{
	UpdateUserList();
}

void UTWLobby_Layout::ShowPlayButton(bool bIsShow)
{
	if (!PlayButton)
	{
		return;
	}

	PlayButton->SetVisibility(bIsShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}