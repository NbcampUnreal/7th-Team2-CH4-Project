#include "UI/Core/TWUIResourceProvider.h"

#include "Engine/World.h"
#include "TimerManager.h"

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
	// 실제 시스템 연동 전까지는 debug fallback 우선
	if (bUseDebugFallback || !ResourceSystemSource)
	{
		return DebugTopBarViewModel;
	}

	// TODO:
	// 실제 ResourceSystemSource 인터페이스/클래스가 정해지면 여기서 실데이터를 읽어오면 됨.
	return DebugTopBarViewModel;
}

int32 UTWUIResourceProvider::GetElapsedSeconds() const
{
	if (bUseDebugFallback || !ResourceSystemSource)
	{
		return DebugElapsedSeconds;
	}

	// TODO:
	// 실제 ResourceSystemSource에서 게임 시간/경과 초를 제공하면 여기서 읽으면 됨.
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

	// 실연동 전에는 debug fallback일 때만 자동 시간 증가
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
	// TODO:
	// 실제 리소스 시스템 인터페이스가 확정되면
	// ResourceSystemSource -> Wood/Gas/Population/ElapsedSeconds 읽고
	// OnResourceChanged.Broadcast() 하도록 연결
}