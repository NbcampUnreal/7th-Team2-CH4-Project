// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleInfoBase.h"
#include "Mass/Replication/BubbleSerializer/TransformSmoothMassClientBubbleSerializer.h"
#include "Net/UnrealNetwork.h"
#include "TransformSmoothMassClientBubbleInfo.generated.h"

UCLASS()
class CH4_PROJECT_API ATransformSmoothMassClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()
  
public:
	ATransformSmoothMassClientBubbleInfo(const FObjectInitializer& ObjectInitializer);

	FTransformSmoothMassClientBubbleSerializer& GetBubbleSerializer() { return BubbleSerializer; }
 
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Contains the entities fast array */
	UPROPERTY(Replicated, Transient) 
	FTransformSmoothMassClientBubbleSerializer BubbleSerializer;
};
