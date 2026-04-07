// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleInfoBase.h"
#include "Mass/Replication/BubbleSerializer/TWTransformSmoothMassClientBubbleSerializer.h"
#include "Net/UnrealNetwork.h"
#include "TWTransformSmoothMassClientBubbleInfo.generated.h"

UCLASS()
class CH4_PROJECT_API ATWTransformSmoothMassClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()
  
public:
	ATWTransformSmoothMassClientBubbleInfo(const FObjectInitializer& ObjectInitializer);

	FTWTransformSmoothMassClientBubbleSerializer& GetBubbleSerializer() { return BubbleSerializer; }
 
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** Contains the entities fast array */
	UPROPERTY(Replicated, Transient) 
	FTWTransformSmoothMassClientBubbleSerializer BubbleSerializer;
};
