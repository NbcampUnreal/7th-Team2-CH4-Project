#include "Building/TWTroopSpawnBuilding.h"

#include "Components/SceneComponent.h"
#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Data/TWBuildingDataAsset.h"
#include "Data/TWTroopBuildingDataAsset.h"
#include "Data/TWUnitTableRowBase.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

ATWTroopSpawnBuilding::ATWTroopSpawnBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));

	if (GetRootComponent())
	{
		SpawnPoint->SetupAttachment(GetRootComponent());
		SpawnPoint->SetRelativeLocation(FVector(200.f, 0.f, 0.f));
	}
	else
	{
		RootComponent = SpawnPoint;
	}
}

void ATWTroopSpawnBuilding::BeginPlay()
{
	Super::BeginPlay();
}

const UTWTroopBuildingDataAsset* ATWTroopSpawnBuilding::GetTroopBuildingData() const
{
	return Cast<UTWTroopBuildingDataAsset>(BuildingData);
}

int8 ATWTroopSpawnBuilding::SpawnUnitNow()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] SpawnUnitNow failed: Not authority"));
		return 0;
	}

	if (!OwningPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] SpawnUnitNow failed: OwningPlayerState is null"));
		return 0;
	}

	if (ProductionQueue.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] SpawnUnitNow failed: ProductionQueue empty"));
		return 0;
	}

	const FName UnitId = ProductionQueue[0];
	const FTWUnitTableRowBase* UnitRow = FindUnitRow(UnitId);
	if (!UnitRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] SpawnUnitNow failed: Unit row not found (%s)"), *UnitId.ToString());
		return 0;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] SpawnUnitNow failed: World is null"));
		return 0;
	}

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] SpawnUnitNow failed: UnitSubsystem is null"));
		return 0;
	}

	ATWPlayerController* OwnerPC = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		if (PC->GetPlayerState<ATWPlayerState>() == OwningPlayerState)
		{
			OwnerPC = Cast<ATWPlayerController>(PC);
			break;
		}
	}

	const FVector SpawnLocation = ResolveSpawnLocation();
	UnitSubsystem->SpawnUnit(SpawnLocation, *UnitRow, OwnerPC);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[TroopSpawnBuilding] SpawnUnitNow success: UnitId=%s / SpawnLocation=%s"),
		*UnitId.ToString(),
		*SpawnLocation.ToString()
	);

	return 1;
}

void ATWTroopSpawnBuilding::SetQueuePausedByUpkeep(const uint8 bInPaused)
{
	if (!HasAuthority())
	{
		return;
	}

	const uint8 NewPausedValue = (bInPaused != 0) ? 1 : 0;
	const uint8 PrevPausedValue = (QueuePausedByUpkeep != 0) ? 1 : 0;

	if (PrevPausedValue == NewPausedValue)
	{
		return;
	}

	QueuePausedByUpkeep = NewPausedValue;
	bQueuePausedByUpkeep = (NewPausedValue != 0);
	ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[TroopSpawnBuilding] QueuePausedByUpkeep changed: %d -> %d"),
		PrevPausedValue,
		NewPausedValue
	);

	if (PrevPausedValue == 1 && NewPausedValue == 0)
	{
		TryStartNextProduction();
	}
}

void ATWTroopSpawnBuilding::StartSpawnQueueTimer()
{
	TryStartNextProduction();
}

void ATWTroopSpawnBuilding::HandleSpawnQueue()
{
	HandleProductionFinished();
}

void ATWTroopSpawnBuilding::ClearAllBuildingTimers()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(SpawnQueueTimerHandle);
	GetWorldTimerManager().ClearTimer(ProductionTimerHandle);

	bIsProducing = false;
	CurrentProducingUnitId = NAME_None;
	CurrentProducingDuration = 0.f;
	CurrentProductionStartTime = 0.f;
	ForceNetUpdate();
}

void ATWTroopSpawnBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWTroopSpawnBuilding, CurrentQueueCount);
	DOREPLIFETIME(ATWTroopSpawnBuilding, QueuePausedByUpkeep);

	DOREPLIFETIME(ATWTroopSpawnBuilding, ProductionQueue);
	DOREPLIFETIME(ATWTroopSpawnBuilding, bIsProducing);
	DOREPLIFETIME(ATWTroopSpawnBuilding, bQueuePausedByUpkeep);
	DOREPLIFETIME(ATWTroopSpawnBuilding, CurrentProducingUnitId);
	DOREPLIFETIME(ATWTroopSpawnBuilding, CurrentProducingDuration);
	DOREPLIFETIME(ATWTroopSpawnBuilding, CurrentProductionStartTime);
}

void ATWTroopSpawnBuilding::OnOwnerPlayerStateAssigned()
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[TroopSpawnBuilding] OnOwnerPlayerStateAssigned"));
	TryStartNextProduction();
}

const UObject* ATWTroopSpawnBuilding::GetTroopBuildingDataObject() const
{
	return BuildingData.Get();
}

bool ATWTroopSpawnBuilding::ResolveTrainableUnitIds(TArray<FName>& OutTrainableUnitIds) const
{
	OutTrainableUnitIds.Reset();

	const UObject* DataObject = GetTroopBuildingDataObject();
	if (!DataObject)
	{
		return false;
	}

	const FArrayProperty* ArrayProperty =
		FindFProperty<FArrayProperty>(DataObject->GetClass(), TEXT("TrainableUnitIds"));
	if (!ArrayProperty)
	{
		return false;
	}

	const FNameProperty* InnerNameProperty = CastField<FNameProperty>(ArrayProperty->Inner);
	if (!InnerNameProperty)
	{
		return false;
	}

	const void* ArrayPtr = ArrayProperty->ContainerPtrToValuePtr<void>(DataObject);
	if (!ArrayPtr)
	{
		return false;
	}

	FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayPtr);
	const int32 Num = ArrayHelper.Num();

	OutTrainableUnitIds.Reserve(Num);

	for (int32 Index = 0; Index < Num; ++Index)
	{
		const void* ElementPtr = ArrayHelper.GetRawPtr(Index);
		if (!ElementPtr)
		{
			continue;
		}

		const FName UnitId = InnerNameProperty->GetPropertyValue(ElementPtr);
		if (!UnitId.IsNone())
		{
			OutTrainableUnitIds.Add(UnitId);
		}
	}

	return OutTrainableUnitIds.Num() > 0;
}

bool ATWTroopSpawnBuilding::ResolveDefaultUnitId(FName& OutUnitId) const
{
	OutUnitId = NAME_None;

	const UObject* DataObject = GetTroopBuildingDataObject();
	if (!DataObject)
	{
		return false;
	}

	const FNameProperty* NameProperty =
		FindFProperty<FNameProperty>(DataObject->GetClass(), TEXT("DefaultUnitId"));
	if (!NameProperty)
	{
		return false;
	}

	OutUnitId = NameProperty->GetPropertyValue_InContainer(DataObject);
	return !OutUnitId.IsNone();
}

int32 ATWTroopSpawnBuilding::ResolveMaxQueueCount() const
{
	const UObject* DataObject = GetTroopBuildingDataObject();
	if (!DataObject)
	{
		return 8;
	}

	const FIntProperty* IntProperty =
		FindFProperty<FIntProperty>(DataObject->GetClass(), TEXT("MaxQueueCount"));
	if (!IntProperty)
	{
		return 8;
	}

	const int32 Value = IntProperty->GetPropertyValue_InContainer(DataObject);
	return (Value > 0) ? Value : 8;
}

const FTWUnitTableRowBase* ATWTroopSpawnBuilding::FindUnitRow(FName UnitId) const
{
	if (UnitId.IsNone())
	{
		return nullptr;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (const UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>())
	{
		return UnitSubsystem->GetUnitTableRowBase(UnitId);
	}

	return nullptr;
}

bool ATWTroopSpawnBuilding::CanEnqueueUnitId(FName UnitId, FString* OutFailReason) const
{
	if (!HasAuthority())
	{
		if (OutFailReason) { *OutFailReason = TEXT("Not authority"); }
		return false;
	}

	if (!OwningPlayerState)
	{
		if (OutFailReason) { *OutFailReason = TEXT("OwningPlayerState is null"); }
		return false;
	}

	if (UnitId.IsNone())
	{
		if (OutFailReason) { *OutFailReason = TEXT("UnitId is none"); }
		return false;
	}

	if (QueuePausedByUpkeep != 0)
	{
		if (OutFailReason) { *OutFailReason = TEXT("Queue paused by upkeep"); }
		return false;
	}

	TArray<FName> TrainableUnitIds;
	if (!ResolveTrainableUnitIds(TrainableUnitIds))
	{
		if (OutFailReason) { *OutFailReason = TEXT("TrainableUnitIds not found"); }
		return false;
	}

	if (!TrainableUnitIds.Contains(UnitId))
	{
		if (OutFailReason) { *OutFailReason = TEXT("UnitId is not trainable by this building"); }
		return false;
	}

	const int32 MaxQueueCount = ResolveMaxQueueCount();
	if (MaxQueueCount > 0 && ProductionQueue.Num() >= MaxQueueCount)
	{
		if (OutFailReason) { *OutFailReason = TEXT("Queue is full"); }
		return false;
	}

	const FTWUnitTableRowBase* UnitRow = FindUnitRow(UnitId);
	if (!UnitRow)
	{
		if (OutFailReason) { *OutFailReason = TEXT("Unit row not found"); }
		return false;
	}

	if (OwningPlayerState->CanAffordCost(UnitRow->Cost) == 0)
	{
		if (OutFailReason) { *OutFailReason = TEXT("Cannot afford cost"); }
		return false;
	}

	if (OwningPlayerState->CanQueueTroop(UnitRow->Population) == 0)
	{
		if (OutFailReason) { *OutFailReason = TEXT("Population limit reached"); }
		return false;
	}

	return true;
}

bool ATWTroopSpawnBuilding::RequestEnqueueTroop()
{
	FName DefaultUnitId = NAME_None;
	if (!ResolveDefaultUnitId(DefaultUnitId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] RequestEnqueueTroop failed: DefaultUnitId not found"));
		return false;
	}

	return RequestEnqueueTroopById(DefaultUnitId);
}

bool ATWTroopSpawnBuilding::RequestEnqueueTroopById(FName UnitId)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] RequestEnqueueTroopById failed: Not authority / UnitId=%s"), *UnitId.ToString());
		return false;
	}

	FString FailReason;
	if (!CanEnqueueUnitId(UnitId, &FailReason))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[TroopSpawnBuilding] RequestEnqueueTroopById failed: UnitId=%s / Reason=%s"),
			*UnitId.ToString(),
			*FailReason
		);
		return false;
	}

	const FTWUnitTableRowBase* UnitRow = FindUnitRow(UnitId);
	if (!UnitRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] RequestEnqueueTroopById failed: Unit row not found (%s)"), *UnitId.ToString());
		return false;
	}

	if (!OwningPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] RequestEnqueueTroopById failed: OwningPlayerState is null"));
		return false;
	}

	OwningPlayerState->SpendCost(UnitRow->Cost);
	OwningPlayerState->AddPendingPopulation(UnitRow->Population);

	ProductionQueue.Add(UnitId);
	CurrentQueueCount = ProductionQueue.Num();
	ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[TroopSpawnBuilding] Enqueue success: UnitId=%s / QueueCount=%d"),
		*UnitId.ToString(),
		CurrentQueueCount
	);

	TryStartNextProduction();
	return true;
}

void ATWTroopSpawnBuilding::TryStartNextProduction()
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] TryStartNextProduction skipped: Not authority"));
		return;
	}

	if (bQueuePausedByUpkeep || QueuePausedByUpkeep != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] TryStartNextProduction skipped: Queue paused by upkeep"));
		return;
	}

	if (bIsProducing)
	{
		return;
	}

	if (ProductionQueue.Num() <= 0)
	{
		return;
	}

	const FName NextUnitId = ProductionQueue[0];
	const FTWUnitTableRowBase* UnitRow = FindUnitRow(NextUnitId);
	if (!UnitRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] TryStartNextProduction failed: Unit row not found (%s)"), *NextUnitId.ToString());
		ProductionQueue.RemoveAt(0);
		CurrentQueueCount = ProductionQueue.Num();
		ForceNetUpdate();
		TryStartNextProduction();
		return;
	}

	StartProductionForUnit(NextUnitId, *UnitRow);
}

void ATWTroopSpawnBuilding::StartProductionForUnit(FName UnitId, const FTWUnitTableRowBase& UnitRow)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bQueuePausedByUpkeep || QueuePausedByUpkeep != 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bIsProducing = true;
	CurrentProducingUnitId = UnitId;
	CurrentProducingDuration = FMath::Max(0.01f, UnitRow.SpawnDuration);
	CurrentProductionStartTime = World->GetTimeSeconds();
	ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[TroopSpawnBuilding] StartProductionForUnit: UnitId=%s / Duration=%.2f"),
		*UnitId.ToString(),
		CurrentProducingDuration
	);

	GetWorldTimerManager().SetTimer(
		ProductionTimerHandle,
		this,
		&ATWTroopSpawnBuilding::HandleProductionFinished,
		CurrentProducingDuration,
		false
	);
}

void ATWTroopSpawnBuilding::HandleProductionFinished()
{
	if (!HasAuthority())
	{
		return;
	}

	auto ResetProductionState = [this]()
	{
		bIsProducing = false;
		CurrentProducingUnitId = NAME_None;
		CurrentProducingDuration = 0.f;
		CurrentProductionStartTime = 0.f;
		ForceNetUpdate();
	};

	if (ProductionQueue.Num() <= 0)
	{
		CurrentQueueCount = 0;
		ResetProductionState();
		return;
	}

	const FName FinishedUnitId = ProductionQueue[0];
	const FTWUnitTableRowBase* UnitRow = FindUnitRow(FinishedUnitId);
	if (!UnitRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] HandleProductionFinished failed: Unit row not found (%s)"), *FinishedUnitId.ToString());
		ProductionQueue.RemoveAt(0);
		CurrentQueueCount = ProductionQueue.Num();
		ResetProductionState();
		TryStartNextProduction();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] HandleProductionFinished failed: World is null"));
		ResetProductionState();
		return;
	}

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] HandleProductionFinished failed: UnitSubsystem is null"));
		ResetProductionState();
		return;
	}

	if (!OwningPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TroopSpawnBuilding] HandleProductionFinished failed: OwningPlayerState is null"));
		ResetProductionState();
		return;
	}

	ATWPlayerController* OwnerPC = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
		if (PS == OwningPlayerState)
		{
			OwnerPC = Cast<ATWPlayerController>(PC);
			break;
		}
	}

	const FVector SpawnLocation = ResolveSpawnLocation();
	UnitSubsystem->SpawnUnit(SpawnLocation, *UnitRow, OwnerPC);

	OwningPlayerState->RemovePendingPopulation(UnitRow->Population);

	ProductionQueue.RemoveAt(0);
	CurrentQueueCount = ProductionQueue.Num();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[TroopSpawnBuilding] HandleProductionFinished success: UnitId=%s / RemainingQueue=%d"),
		*FinishedUnitId.ToString(),
		CurrentQueueCount
	);

	ResetProductionState();
	TryStartNextProduction();
}

float ATWTroopSpawnBuilding::GetCurrentProductionProgressRatio() const
{
	if (!bIsProducing || CurrentProducingUnitId.IsNone() || CurrentProducingDuration <= 0.f)
	{
		return 0.f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	const float Elapsed = World->GetTimeSeconds() - CurrentProductionStartTime;
	return FMath::Clamp(Elapsed / CurrentProducingDuration, 0.f, 1.f);
}

FString ATWTroopSpawnBuilding::GetCurrentProductionProgressText() const
{
	const float Ratio = GetCurrentProductionProgressRatio();
	return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Ratio * 100.f));
}

FVector ATWTroopSpawnBuilding::ResolveSpawnLocation() const
{
	if (SpawnPoint)
	{
		return SpawnPoint->GetComponentLocation();
	}

	return GetActorLocation() + (GetActorForwardVector() * 200.f);
}