#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "TWSoundAsset.generated.h"


USTRUCT(BlueprintType)
struct FSoundTagData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SoundTag")
	FGameplayTag SoundTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SoundTag")
	TObjectPtr<USoundBase> SoundAsset;
};

UCLASS()
class CH4_PROJECT_API UTWSoundAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("SoundConfig", GetFName());
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Sound")
	TArray<FSoundTagData> SoundMappings;
	
	USoundBase* GetSoundByTag(const FGameplayTag& InTag) const;
};
