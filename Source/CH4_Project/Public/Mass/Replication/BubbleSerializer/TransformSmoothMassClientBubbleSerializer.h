// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleSerializerBase.h"
#include "TransformClientBubbleSerializer.h"
#include "Mass/Replication/BubbleHandler/TransformSmoothMassClientBubbleHandler.h"
#include "UObject/Object.h"
#include "TransformSmoothMassClientBubbleSerializer.generated.h"

/**
 * 
 */
USTRUCT()
struct  CH4_PROJECT_API FTransformSmoothMassClientBubbleSerializer : public FMassClientBubbleSerializerBase
{
	GENERATED_BODY()
public:
	FTransformSmoothMassClientBubbleSerializer()
	{
		Bubble.Initialize(Entities, *this);
	}
 
	/** Define a custom replication for this struct */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FTransformMassFastArrayItem, FTransformSmoothMassClientBubbleSerializer>(Entities, DeltaParams, *this);
	}
 
	/** The one responsible for storing the server data in the client fragments */
	UTransformSmoothMassClientBubbleHandler Bubble;
protected:
	/** Fast Array of Agents for efficient replication. Maintained as a freelist on the server, to keep index consistency as indexes are used as Handles into the Array 
	 *  Note array order is not guaranteed between server and client so handles will not be consistent between them, FMassNetworkID will be.*/
	UPROPERTY(Transient)
	TArray<FTransformMassFastArrayItem> Entities;
	
};

template<>
struct TStructOpsTypeTraits<FTransformSmoothMassClientBubbleSerializer> : public TStructOpsTypeTraitsBase2<FTransformClientBubbleSerializer>
{
	enum
	{
		// We need to use the NetDeltaSerialize function for this struct to define a custom replication
		WithNetDeltaSerializer = true,
 
		// Copy is not allowed for this struct
		WithCopy = false,
	};
};