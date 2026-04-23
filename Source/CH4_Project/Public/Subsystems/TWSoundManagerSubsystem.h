#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "TWSoundManagerSubsystem.generated.h"

class UTWSoundAsset;

UCLASS()
class CH4_PROJECT_API UTWSoundManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void PlaySoundByTag(FGameplayTag SoundTag, FVector Location, AActor* ContextActor = nullptr);

private:
	
	UPROPERTY()
	TObjectPtr<UTWSoundAsset> CachedSoundData;
	
	UTWSoundAsset* GetSoundData();

	bool IsLocationInCameraView(FVector Location);
};
