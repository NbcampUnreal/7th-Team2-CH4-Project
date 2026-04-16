// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWTitlePlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CH4_PROJECT_API ATWTitlePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	void JoinServer(const FString& InIPAddress) const;
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ASUIPLayerController, Meta = (AllowPrivateAccess))
	TSubclassOf<UUserWidget> UIWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = ASUIPLayerController, Meta = (AllowPrivateAccess))
	TObjectPtr<UUserWidget> UIWidgetInstance;
};

