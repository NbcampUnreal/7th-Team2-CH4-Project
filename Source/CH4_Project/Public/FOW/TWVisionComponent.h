// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TWVisionComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), DisplayName="Fog Vision")
class CH4_PROJECT_API UTWVisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTWVisionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vision")
	float VisionRadius = 1000.f;
	
	FVector GetVisionLocation() const;
};
