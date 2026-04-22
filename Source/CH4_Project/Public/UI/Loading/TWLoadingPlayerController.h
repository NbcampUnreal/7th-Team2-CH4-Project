// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWLoadingPlayerController.generated.h"

class UUserWidget;

UCLASS()
class CH4_PROJECT_API ATWLoadingPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loading")
	TSubclassOf<UUserWidget> LoadingWidgetClass;
};
