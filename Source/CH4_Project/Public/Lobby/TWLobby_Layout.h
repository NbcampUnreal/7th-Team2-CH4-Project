// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWLobby_Layout.generated.h"

class UButton;
class UHorizontalBox;
class UImage;
class UEditableText;
class UComboBoxString;
class UCommonTextBlock;

UCLASS()
class CH4_PROJECT_API UTWLobby_Layout : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UTWLobby_Layout(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:
	virtual void NativeConstruct() override;
	
	UFUNCTION()
	void OnPlayButtonClicked();
	
	UFUNCTION()
	void OnReadyButtonClicked();
	
	UFUNCTION()
	void OnCancelButtonClicked();
	
	UFUNCTION()
	void OnLobbyExitButtonClicked();

	UFUNCTION()
	void OnApplySettingsButtonClicked();

	UFUNCTION()
	void DeferredRefreshUserList();

	FString GetDisplayHeroName(const FName& InHeroUnitId) const;
	void RefreshUserSlot(int32 SlotIndex);
	
public:
	UFUNCTION(BlueprintCallable)
	void SubmitNickname(const FString& InNickname);

	UFUNCTION(BlueprintCallable)
	void SelectHero(FName InHeroUnitId);
	
	UFUNCTION()
	void UpdateUserList();
	
	UFUNCTION()
	void UpdateUserImage();
	
	void ShowPlayButton(bool bIsShow);
	
protected:
#pragma region 버튼 연결
	UPROPERTY(EditDefaultsOnly, Category = USLobyWidget, meta = (BindWidget))
	TObjectPtr<UButton> PlayButton;
	
	UPROPERTY(EditDefaultsOnly, Category = USLobyWidget, meta = (BindWidget))
	TObjectPtr<UButton> ReadyButton;
	
	UPROPERTY(EditDefaultsOnly, Category = USLobyWidget, meta = (BindWidget))
	TObjectPtr<UButton> CancelButton;
	
	UPROPERTY(EditDefaultsOnly, Category = USLobyWidget, meta = (BindWidget))
	TObjectPtr<UButton> LobbyExitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplySettingsButton;
#pragma endregion

#pragma region 설정 패널 연결
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableText> NickNameInput;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> HeroSelectComboBox;
#pragma endregion
	
#pragma region 이름 및 상태 표시 연결
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PlayerName1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> HeroText1;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage1;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PlayerName2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> HeroText2;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage2;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PlayerName3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> HeroText3;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage3;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage3;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> PlayerName4;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonTextBlock> HeroText4;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage4;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage4;
#pragma endregion
	
	UPROPERTY()
	TArray<TObjectPtr<UHorizontalBox>> NickNameSlots;

	UPROPERTY()
	TArray<TObjectPtr<UCommonTextBlock>> PlayerNameTexts;

	UPROPERTY()
	TArray<TObjectPtr<UCommonTextBlock>> HeroTexts;
	
	UPROPERTY()
	TArray<TObjectPtr<UImage>> HostImages;
	
	UPROPERTY()
	TArray<TObjectPtr<UImage>> ReadyImages;

	UPROPERTY()
	FTimerHandle DeferredRefreshTimerHandle;
};