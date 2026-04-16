// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/TWTitle_Layout.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Title/TWTitlePlayerController.h"

UTWTitle_Layout::UTWTitle_Layout(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UTWTitle_Layout::NativeConstruct()
{
	StartButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnStartButtonClicked);
	NickNameIPEditableText->OnTextCommitted.AddDynamic(this, &ThisClass::OnNickNameCommitted);
	ExitButton.Get()->OnClicked.AddDynamic(this, &ThisClass::OnExitButtonClicked);
}

void UTWTitle_Layout::OnStartButtonClicked()
{
	if (NickNameIPEditableText->GetIsEnabled() == false)
	{
		NickNameIPEditableText->SetIsEnabled(true);
		NickNameIPEditableText->SetKeyboardFocus();
		return;
	}
	
	FString UserID = NickNameIPEditableText->GetText().ToString();
	
	if (UserID.TrimStartAndEnd().IsEmpty() == false)
	{
		// 이 위젯을 소유하고 있는 플레이어 컨트롤러를 가져와서, 우리가 정의한 ATitlePlayerController 타입으로 형변환(Casting)한다.
		// TitlePlayerController.cpp에서 생성한 JoinServer 함수를 호출하기 위해 컨트롤러의 주소값이 필요하기 때문이다.
		ATWTitlePlayerController* PC = GetOwningPlayer<ATWTitlePlayerController>();
	
		// 가져온 컨트롤러가 존재하는지(Null이 아닌지)를 확인한다.
		if (IsValid(PC) == true)
		{
			// 사용자가 UI의 입력창(EditableText)에 타이핑한 텍스트를 가져온다.
			FString ServerIP = TEXT("127.0.0.1:17777");
			// 가져온 IP를 문자열(FString)로 변환하여 컨트롤러의 JoinServer 함수에 전달한다.
			FString URL = FString::Printf(TEXT("%s?Name=%s"), *ServerIP, *UserID);
			PC->JoinServer(URL);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("!!!!!닉네임을 입력해주세요!!!!!"));
	}
}

void UTWTitle_Layout::OnExitButtonClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UTWTitle_Layout::OnNickNameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		OnStartButtonClicked();
	}
}
