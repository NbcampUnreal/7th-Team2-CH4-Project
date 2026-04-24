// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/TWSoundAsset.h"


USoundBase* UTWSoundAsset::GetSoundByTag(const FGameplayTag& InTag) const
{
	for (const auto& Mapping : SoundMappings)
	{
		if (Mapping.SoundTag.MatchesTagExact(InTag))
		{
			return Mapping.SoundAsset;
		}
	}
	return nullptr;
}
