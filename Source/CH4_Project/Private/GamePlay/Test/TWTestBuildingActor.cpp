#include "Gameplay/Test/TWTestBuildingActor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Gameplay/Test/TWTestUnitActor.h"
#include "TimerManager.h"

ATWTestBuildingActor::ATWTestBuildingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(SceneRoot);
	SpawnPoint->SetRelativeLocation(FVector(200.f, 0.f, 0.f));
}

void ATWTestBuildingActor::BeginPlay()
{
	Super::BeginPlay();
}

FName ATWTestBuildingActor::GetUIEntityId() const
{
	return BuildingId;
}

FString ATWTestBuildingActor::GetUIDisplayName() const
{
	return DisplayName;
}

FString ATWTestBuildingActor::GetUITypeLabel() const
{
	return TypeLabel;
}

float ATWTestBuildingActor::GetCurrentHP() const
{
	return CurrentHPValue;
}

float ATWTestBuildingActor::GetMaxHP() const
{
	return MaxHPValue;
}

TArray<FName> ATWTestBuildingActor::GetAvailableCommandIds() const
{
	return AvailableCommandIds;
}

int32 ATWTestBuildingActor::GetQueueCountForCommand(const FName& InCommandId) const
{
	if (InCommandId == TEXT("TrainFootman"))
	{
		return FootmanQueueCount;
	}

	if (InCommandId == TEXT("TrainArcher"))
	{
		return ArcherQueueCount;
	}

	return 0;
}

bool ATWTestBuildingActor::CanTrainFootman() const
{
	return !bIsTraining && FootmanClass != nullptr;
}

bool ATWTestBuildingActor::StartTrainFootman()
{
	if (!CanTrainFootman())
	{
		return false;
	}

	bIsTraining = true;
	++FootmanQueueCount;

	GetWorldTimerManager().SetTimer(
		TrainHandle,
		this,
		&ATWTestBuildingActor::FinishTrainFootman,
		TrainDuration,
		false
	);

	return true;
}

bool ATWTestBuildingActor::CanTrainArcher() const
{
	return !bIsTraining && ArcherClass != nullptr;
}

bool ATWTestBuildingActor::StartTrainArcher()
{
	if (!CanTrainArcher())
	{
		return false;
	}

	bIsTraining = true;
	++ArcherQueueCount;

	GetWorldTimerManager().SetTimer(
		TrainHandle,
		this,
		&ATWTestBuildingActor::FinishTrainArcher,
		TrainDuration,
		false
	);

	return true;
}

void ATWTestBuildingActor::FinishTrainFootman()
{
	if (UWorld* World = GetWorld())
	{
		if (FootmanClass)
		{
			const FVector SpawnLocation = SpawnPoint
				? SpawnPoint->GetComponentLocation()
				: GetActorLocation() + FVector(200.f, 0.f, 0.f);

			World->SpawnActor<ATWTestUnitActor>(FootmanClass, SpawnLocation, GetActorRotation());
		}
	}

	FootmanQueueCount = FMath::Max(0, FootmanQueueCount - 1);
	bIsTraining = false;
}

void ATWTestBuildingActor::FinishTrainArcher()
{
	if (UWorld* World = GetWorld())
	{
		if (ArcherClass)
		{
			const FVector SpawnLocation = SpawnPoint
				? SpawnPoint->GetComponentLocation()
				: GetActorLocation() + FVector(200.f, 0.f, 0.f);

			World->SpawnActor<ATWTestUnitActor>(ArcherClass, SpawnLocation, GetActorRotation());
		}
	}

	ArcherQueueCount = FMath::Max(0, ArcherQueueCount - 1);
	bIsTraining = false;
}