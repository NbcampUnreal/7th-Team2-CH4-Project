// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TWLobby_Layout.generated.h"

class UButton;
class UVerticalBox;
class UHorizontalBox;

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
	
	void ShowPlayButton(bool bIsShow);
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USLobyWidget, meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> PlayButton;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USLobyWidget, meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> ReadyButton;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USLobyWidget, meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> CancelButton;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = USLobyWidget, meta = (AllowPrivateAccess, BindWidget))
	TObjectPtr<UButton> LobbyExitButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_3;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> NickName_4;
	
	
	UPROPERTY()
	TArray<UHorizontalBox*> NickNameSlots;
};

