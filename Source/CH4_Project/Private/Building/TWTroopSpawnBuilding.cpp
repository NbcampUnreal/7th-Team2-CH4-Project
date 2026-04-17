#include "Building/TWTroopSpawnBuilding.h"

#include "Components/SceneComponent.h"
#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Data/TWBuildingDataAsset.h"
#include "Data/TWUnitTableRowBase.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "GameFramework/GameStateBase.h"

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

void ATWTroopSpawnBuilding::SetQueuePausedByUpkeep(bool bInPaused)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (BuildingState != ETWBuildingState::Completed)
	{
		return;
	}
    
	const bool bPrevPaused = bQueuePausedByUpkeep;
	if (bPrevPaused == bInPaused)
	{
		return;
	}

	bQueuePausedByUpkeep = bInPaused;
	ForceNetUpdate();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[TroopSpawnBuilding] QueuePausedByUpkeep changed: %d -> %d"),
		bPrevPaused ? 1 : 0,
		bInPaused ? 1 : 0
	);
	
	if (bPrevPaused && !bInPaused)
	{
		TryStartNextProduction();
	}
}

void ATWTroopSpawnBuilding::CancelQueuedProductionAndRestorePendingPopulation()
{
	if (!HasAuthority())
	{
		return;
	}

	int32 TotalPendingPopulationToRestore = 0;

	for (const FName& UnitId : ProductionQueue)
	{
		const FTWUnitTableRowBase* UnitRow = FindUnitRow(UnitId);
		if (!UnitRow)
		{
			continue;
		}

		TotalPendingPopulationToRestore += UnitRow->Population;
	}

	if (OwningPlayerState && TotalPendingPopulationToRestore > 0)
	{
		OwningPlayerState->RemovePendingPopulation(TotalPendingPopulationToRestore);
	}

	ProductionQueue.Empty();
}

void ATWTroopSpawnBuilding::ClearAllBuildingTimers()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ProductionTimerHandle);

	CancelQueuedProductionAndRestorePendingPopulation();
	
	bIsProducing = false;
	bQueuePausedByUpkeep = false;
	CurrentProducingUnitId = NAME_None;
	CurrentProducingDuration = 0.f;
	CurrentProductionStartTime = 0.f;
	ForceNetUpdate();
}

void ATWTroopSpawnBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
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
	
	if (BuildingState != ETWBuildingState::Completed)
	{
		if (OutFailReason) { *OutFailReason = TEXT("Building is not completed"); }
		return false;
	}

	if (UnitId.IsNone())
	{
		if (OutFailReason) { *OutFailReason = TEXT("UnitId is none"); }
		return false;
	}

	if (bQueuePausedByUpkeep != 0)
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
		UE_LOG(LogTemp, Warning, TEXT("[병력 생산 건물] 대기열 추가 실패: 기본 유닛 ID를 찾을 수 없음"));
		return false;
	}

	return RequestEnqueueTroopById(DefaultUnitId);
}

bool ATWTroopSpawnBuilding::RequestEnqueueTroopById(FName UnitId)
{
	FString FailReason;
	if (!CanEnqueueUnitId(UnitId, &FailReason))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[병력 생산 건물] 대기열 추가 실패 | UnitID: %s | 사유: %s"),
			*UnitId.ToString(),
			*FailReason
		);
		return false;
	}

	const FTWUnitTableRowBase* UnitRow = FindUnitRow(UnitId);
	if (!UnitRow || !OwningPlayerState)
	{
		return false;
	}

	OwningPlayerState->SpendCost(UnitRow->Cost);
	OwningPlayerState->AddPendingPopulation(UnitRow->Population);

	ProductionQueue.Add(UnitId);
	ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[병력 생산 건물] 대기열 추가 성공 | UnitID: %s | 현재 대기열 수: %d"),
		*UnitId.ToString(),
		ProductionQueue.Num()
	);

	TryStartNextProduction();
	return true;
}

void ATWTroopSpawnBuilding::TryStartNextProduction()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bQueuePausedByUpkeep)
	{
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

	if (bQueuePausedByUpkeep)
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
	const AGameStateBase* GS = World->GetGameState();
	CurrentProductionStartTime = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[병력 생산 건물] 생산 시작 | UnitID: %s | 생산 시간: %.2f초"),
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
		ResetProductionState();
		return;
	}

	const FName FinishedUnitId = ProductionQueue[0];
	const FTWUnitTableRowBase* UnitRow = FindUnitRow(FinishedUnitId);
	if (!UnitRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[병력 생산 건물] 생산 완료 처리 실패: 유닛 데이터 행을 찾을 수 없음 | UnitID: %s"), *FinishedUnitId.ToString());
		ProductionQueue.RemoveAt(0);
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

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[병력 생산 건물] 생산 완료 | UnitID: %s | 남은 대기열 수: %d"),
		*FinishedUnitId.ToString(),
		ProductionQueue.Num()
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

	const AGameStateBase* GS = World->GetGameState();
	const float Now = GS ? GS->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	
	const float Elapsed = FMath::Max(0.f, Now - CurrentProductionStartTime);
	return FMath::Clamp(Elapsed / CurrentProducingDuration, 0.f, 1.f);
}

FString ATWTroopSpawnBuilding::GetCurrentProductionProgressText() const
{
	const float Ratio = GetCurrentProductionProgressRatio();
	return FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Ratio * 100.f));
}

FVector ATWTroopSpawnBuilding::ResolveSpawnLocation() const
{
	if (SpawnPoint)
	{
		return SpawnPoint->GetComponentLocation();
	}

	return GetActorLocation() + (GetActorForwardVector() * 200.f);
}