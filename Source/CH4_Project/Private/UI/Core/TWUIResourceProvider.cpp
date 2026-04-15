#include "UI/Core/TWUIResourceProvider.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Data/TWBuildingTypes.h"

namespace
{
	static const ATWPlayerState* ResolvePlayerStateFromSource(const UObject* InSource)
	{
		if (!InSource)
		{
			return nullptr;
		}

		if (const ATWPlayerState* TWPS = Cast<ATWPlayerState>(InSource))
		{
			return TWPS;
		}

		if (const ATWPlayerController* TWPC = Cast<ATWPlayerController>(InSource))
		{
			return TWPC->GetPlayerState<ATWPlayerState>();
		}

		return nullptr;
	}
}

UTWUIResourceProvider::UTWUIResourceProvider()
{
	DebugTopBarViewModel = FTopBarViewModel();
	DebugElapsedSeconds = 0;
}

UWorld* UTWUIResourceProvider::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

void UTWUIResourceProvider::Initialize(UObject* InResourceSystemSource)
{
	ResourceSystemSource = InResourceSystemSource;
	ApplyAutoAdvanceState();
}

void UTWUIResourceProvider::Shutdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
	}

	ResourceSystemSource = nullptr;
}

void UTWUIResourceProvider::SetUseDebugFallback(bool bInUseDebugFallback)
{
	bUseDebugFallback = bInUseDebugFallback;
	ApplyAutoAdvanceState();
	OnResourceChanged.Broadcast();
}

bool UTWUIResourceProvider::IsUsingDebugFallback() const
{
	return bUseDebugFallback;
}

void UTWUIResourceProvider::DebugSetResources(
	const FTopBarViewModel& InTopBarViewModel,
	int32 InElapsedSeconds)
{
	DebugTopBarViewModel = InTopBarViewModel;
	DebugElapsedSeconds = FMath::Max(0, InElapsedSeconds);

	ApplyAutoAdvanceState();
	OnResourceChanged.Broadcast();
}

void UTWUIResourceProvider::SetAutoAdvanceTime(bool bInEnabled)
{
	bAutoAdvanceTime = bInEnabled;
	ApplyAutoAdvanceState();
}

void UTWUIResourceProvider::SetAutoAdvanceInterval(float InSeconds)
{
	AutoAdvanceInterval = FMath::Max(0.05f, InSeconds);
	ApplyAutoAdvanceState();
}

FTopBarViewModel UTWUIResourceProvider::GetTopBarViewModel_Implementation() const
{
	if (bUseDebugFallback || !ResourceSystemSource)
	{
		return DebugTopBarViewModel;
	}

	const ATWPlayerState* TWPS = ResolvePlayerStateFromSource(ResourceSystemSource);
	if (!TWPS)
	{
		return DebugTopBarViewModel;
	}

	FTopBarViewModel VM = DebugTopBarViewModel;

	const int32 Wood = TWPS->GetResourceAmount(EResourceType::Wood);
	const int32 Gas = TWPS->GetResourceAmount(EResourceType::Ore);
	const int32 CurrentPopulation = TWPS->GetCurrentPopulation();
	const int32 PendingPopulation = TWPS->GetPendingPopulation();
	const int32 PopulationLimit = TWPS->GetPopulationLimit();
	const int32 MaxPopulation = TWPS->GetMaxPopulation();
	const int32 DisplayPopulation = CurrentPopulation + PendingPopulation;

	VM.Wood = Wood;
	VM.Gas = Gas;
	VM.Population = DisplayPopulation;
	VM.PopulationText = FString::Printf(TEXT("%d / %d"), DisplayPopulation, PopulationLimit);

	VM.CurrentPopulation = CurrentPopulation;
	VM.PendingPopulation = PendingPopulation;
	VM.PopulationLimit = PopulationLimit;
	VM.MaxPopulation = MaxPopulation;

	return VM;
}

int32 UTWUIResourceProvider::GetElapsedSeconds() const
{
	if (bUseDebugFallback || !ResourceSystemSource)
	{
		return DebugElapsedSeconds;
	}

	if (const ATWPlayerController* TWPC = Cast<ATWPlayerController>(ResourceSystemSource))
	{
		if (const UWorld* World = TWPC->GetWorld())
		{
			return FMath::FloorToInt(World->GetTimeSeconds());
		}
	}

	if (const ATWPlayerState* TWPS = Cast<ATWPlayerState>(ResourceSystemSource))
	{
		if (const UWorld* World = TWPS->GetWorld())
		{
			return FMath::FloorToInt(World->GetTimeSeconds());
		}
	}

	return DebugElapsedSeconds;
}

FOnUIResourceChanged& UTWUIResourceProvider::GetOnResourceChangedDelegate()
{
	return OnResourceChanged;
}

void UTWUIResourceProvider::ApplyAutoAdvanceState()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();
	TimerManager.ClearTimer(AutoAdvanceTimerHandle);

	if (bUseDebugFallback && bAutoAdvanceTime)
	{
		TimerManager.SetTimer(
			AutoAdvanceTimerHandle,
			this,
			&UTWUIResourceProvider::HandleAutoAdvanceTick,
			AutoAdvanceInterval,
			true);
	}
}

void UTWUIResourceProvider::HandleAutoAdvanceTick()
{
	if (!bUseDebugFallback)
	{
		return;
	}

	++DebugElapsedSeconds;
	OnResourceChanged.Broadcast();
}

void UTWUIResourceProvider::RefreshFromRealSourceIfAvailable()
{
	if (bUseDebugFallback || !ResourceSystemSource)
	{
		return;
	}

	const ATWPlayerState* TWPS = ResolvePlayerStateFromSource(ResourceSystemSource);
	if (!TWPS)
	{
		return;
	}

	OnResourceChanged.Broadcast();
}