// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWLobby_Layout.generated.h"

class UButton;
class UVerticalBox;
class UHorizontalBox;
class UImage;

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
	
public:
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
#pragma endregion
	
#pragma region 이름 및 상태 표시 연결
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_1;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage1;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_2;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage2;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_3;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage3;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage3;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_4;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HostImage4;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ReadyImage4;
#pragma endregion
	
	UPROPERTY()
	TArray<UHorizontalBox*> NickNameSlots;
	
	UPROPERTY()
	TArray<UImage*> HostImages;
	
	UPROPERTY()
	TArray<UImage*> ReadyImages;
};

