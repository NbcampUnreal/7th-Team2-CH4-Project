// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleInfoBase.h"
#include "Mass/Replication/BubbleSerializer/TWClientBubbleSerializer.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TWMassClientBubbleInfo.generated.h"

UCLASS()
class CH4_PROJECT_API ATWMassClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()

	
public:
	ATWMassClientBubbleInfo(const FObjectInitializer& ObjectInitializer) 
		:Super(ObjectInitializer)
	{
		// Adding our serializer to this array so our parent class can initialize it
		Serializers.Add(&BubbleSerializer);
	}
 
	FTWClientBubbleSerializer& GetBubbleSerializer() { return BubbleSerializer; }
 
protected:
 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated, Transient) 
	FTWClientBubbleSerializer BubbleSerializer;
};
