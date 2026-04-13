// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleInfoBase.h"
#include "Mass/Replication/BubbleSerializer/TWStatusClientBubbleSerializer.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "TWStatusMassClientBubbleInfo.generated.h"

UCLASS()
class CH4_PROJECT_API ATWStatusMassClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()

	
public:
	ATWStatusMassClientBubbleInfo(const FObjectInitializer& ObjectInitializer) 
		:Super(ObjectInitializer)
	{
		// Adding our serializer to this array so our parent class can initialize it
		Serializers.Add(&BubbleSerializer);
	}
 
	FTWStatusClientBubbleSerializer& GetBubbleSerializer() { return BubbleSerializer; }
 
protected:
 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(Replicated, Transient) 
	FTWStatusClientBubbleSerializer BubbleSerializer;
};
