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

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlaySoundAttached(FGameplayTag SoundTag, USceneComponent* AttachToComponent);

	UFUNCTION(BlueprintCallable, Category = "Sound")
	UAudioComponent* PlaySoundLoopAttached(FGameplayTag SoundTag, USceneComponent* AttachToComponent);

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void StopSoundLoop(UAudioComponent* LoopComponent, float FadeOutTime = 0.5f);
	
private:
	
	UPROPERTY()
	TObjectPtr<UTWSoundAsset> CachedSoundData;
	
	UTWSoundAsset* GetSoundData();

	bool IsLocationInCameraView(FVector Location);
};
