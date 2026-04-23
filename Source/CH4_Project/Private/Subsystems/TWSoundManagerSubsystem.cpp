#include "Subsystems/TWSoundManagerSubsystem.h"

#include "Data/TWSoundAsset.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"

void UTWSoundManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	GetSoundData();
}

void UTWSoundManagerSubsystem::PlaySoundByTag(FGameplayTag SoundTag, FVector Location, AActor* ContextActor)
{
	if (!IsLocationInCameraView(Location))
	{
		return; 
	}
	
	UTWSoundAsset* Data = GetSoundData();
	if (!Data)
	{
		return;
	}
	
	if (USoundBase* Sound = Data->GetSoundByTag(SoundTag))
	{
		UWorld* World = ContextActor ? ContextActor->GetWorld() : GetGameInstance()->GetWorld();
		if (World)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World, 
				Sound,	
				Location,
				1.0f,	// 볼륨 배수
				1.0f,	// Pitch 배수
				0.0f	// 시작 시간
				);
		}
	}
}

UTWSoundAsset* UTWSoundManagerSubsystem::GetSoundData()
{
	if (CachedSoundData)
	{
		return CachedSoundData;
	}
	
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
	{
		return nullptr;
	}
	
	TArray<FPrimaryAssetId> AssetIdList;
	AssetManager->GetPrimaryAssetIdList(FPrimaryAssetType("SoundConfig"), AssetIdList);
	
	if (AssetIdList.Num() > 0)
	{
		FPrimaryAssetId TargetId = AssetIdList[0];
		CachedSoundData = Cast<UTWSoundAsset>(AssetManager->GetPrimaryAssetObject(TargetId));
		
		if (!CachedSoundData)
		{
			FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(TargetId);
			
			CachedSoundData = Cast<UTWSoundAsset>(AssetPath.TryLoad());
		}
	}
	return CachedSoundData;
	
}

bool UTWSoundManagerSubsystem::IsLocationInCameraView(FVector Location)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return false;
	}
	
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return false;
	}
	
	FVector2D ScreenPosition;
	if (UGameplayStatics::ProjectWorldToScreen(PC, Location, ScreenPosition))
	{
		int32 SizeX, SizeY;
		PC->GetViewportSize(SizeX, SizeY);
		
		return (ScreenPosition.X >= 0 && ScreenPosition.X <= SizeX &&
				ScreenPosition.Y >= 0 && ScreenPosition.Y <= SizeY);
	}
	
	return false;
}
