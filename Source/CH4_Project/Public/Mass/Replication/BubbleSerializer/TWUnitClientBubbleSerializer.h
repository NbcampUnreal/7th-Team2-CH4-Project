// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassClientBubbleSerializerBase.h"
#include "Mass/Replication/BubbleHandler/TWUnitMassClientBubbleHandler.h"
#include "Mass/Replication/FastArrayItem/TWUnitMassFastArrayItem.h"
#include "TWUnitClientBubbleSerializer.generated.h"

/**
 * 
 */
USTRUCT()
struct CH4_PROJECT_API FTWUnitClientBubbleSerializer : public FMassClientBubbleSerializerBase
{
	GENERATED_BODY()
public:
	FTWUnitClientBubbleSerializer()
	{
		Bubble.Initialize(Entities, *this);
	}
 
	/** Define a custom replication for this struct */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FTWUnitMassFastArrayItem, FTWUnitClientBubbleSerializer>(Entities, DeltaParams, *this);
	}
 
	/** The one responsible for storing the server data in the client fragments */
	FTWUnitMassClientBubbleHandler Bubble;
 
protected:
	/** Fast Array of Agents for efficient replication. Maintained as a freelist on the server, to keep index consistency as indexes are used as Handles into the Array 
	 *  Note array order is not guaranteed between server and client so handles will not be consistent between them, FMassNetworkID will be.
	 */
	UPROPERTY(Transient)
	TArray<FTWUnitMassFastArrayItem> Entities;
	
};

template<>
struct TStructOpsTypeTraits<FTWUnitClientBubbleSerializer> : public TStructOpsTypeTraitsBase2<FTWUnitClientBubbleSerializer>
{
	enum
	{
		// We need to use the NetDeltaSerialize function for this struct to define a custom replication
		WithNetDeltaSerializer = true,
 
		// Copy is not allowed for this struct
		WithCopy = false,
	};
};
