#include "Subsystems/TWSoundManagerSubsystem.h"
#include "Data/TWSoundAsset.h"
#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

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

void UTWSoundManagerSubsystem::PlayUISoundByTag(FGameplayTag SoundTag)
{
	if (UTWSoundAsset* Data = GetSoundData())
	{
		if (USoundBase* Sound = Data->GetSoundByTag(SoundTag))
		{
			if (UWorld* World = GetWorld())
			{
				UGameplayStatics::PlaySound2D(World, Sound);
			}
		}
	}
}

void UTWSoundManagerSubsystem::PlaySoundAttached(FGameplayTag SoundTag, USceneComponent* AttachToComponent)
{
	if (!AttachToComponent)
	{
		return;
	}
	
	if (UTWSoundAsset* Data = GetSoundData())
	{
		if (USoundBase* Sound = Data->GetSoundByTag(SoundTag))
		{
			UGameplayStatics::SpawnSoundAttached(
				Sound, 
				AttachToComponent, 
				NAME_None, 
				FVector::ZeroVector, 
				EAttachLocation::KeepRelativeOffset, 
				true
			);
		}
	}
}

UAudioComponent* UTWSoundManagerSubsystem::PlaySoundLoopAttached(FGameplayTag SoundTag,
	USceneComponent* AttachToComponent)
{
	if (!AttachToComponent)
	{
		return nullptr;
	}

	if (UTWSoundAsset* Data = GetSoundData())
	{
		if (USoundBase* Sound = Data->GetSoundByTag(SoundTag))
		{
			UAudioComponent* LoopComp = UGameplayStatics::SpawnSoundAttached(
				Sound, AttachToComponent, NAME_None, FVector::ZeroVector, 
				EAttachLocation::KeepRelativeOffset, false
			);
			return LoopComp;
		}
	}
	return nullptr;
}

void UTWSoundManagerSubsystem::StopSoundLoop(UAudioComponent* LoopComponent, float FadeOutTime)
{
	if (!IsValid(LoopComponent))
	{
		return;
	}

	if (FadeOutTime > 0.f)
	{
		LoopComponent->bAutoDestroy = true;
		LoopComponent->FadeOut(FadeOutTime, 0.f);
	}
	else
	{
		LoopComponent->Stop();
		LoopComponent->DestroyComponent();
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
