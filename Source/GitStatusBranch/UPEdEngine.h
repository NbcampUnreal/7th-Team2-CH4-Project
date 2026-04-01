// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Editor/UnrealEdEngine.h"
#include "UPEdEngine.generated.h"

/**
 * 
 */
UCLASS()
class GITSTATUSBRANCH_API UUPEdEngine : public UUnrealEdEngine
{
	GENERATED_BODY()
	
public:
	virtual void Init(IEngineLoop* InEngineLoop) override;
};
