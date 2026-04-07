// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleInfoBase.h"
#include "Mass/Replication/BubbleSerializer/TWTransformClientBubbleSerializer.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TWTransformMassClientBubbleInfo.generated.h"

UCLASS()
class CH4_PROJECT_API ATWTransformMassClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()

	
public:
	ATWTransformMassClientBubbleInfo(const FObjectInitializer& ObjectInitializer) 
		:Super(ObjectInitializer)
	{
		// Adding our serializer to this array so our parent class can initialize it
		Serializers.Add(&BubbleSerializer);
	}
 
	FTWTransformClientBubbleSerializer& GetBubbleSerializer() { return BubbleSerializer; }
 
protected:
 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated, Transient) 
	FTWTransformClientBubbleSerializer BubbleSerializer;
};
