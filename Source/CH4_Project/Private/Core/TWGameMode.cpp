#include "Core/TWGameMode.h"

#include "Core/TWPlayerState.h"
#include "Core/TWPlayerController.h"
#include "Building/TWBaseBuilding.h"
#include "Building/TWNexusBuilding.h"
#include "Subsystems/TWUnitSubsystem.h"
#include "Component/TWTeamComponent.h"
#include "HeroUnit/TWHeroUnitBase.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

void ATWGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Seamless Travel로 넘어온 기존 플레이어는 HandleSeamlessTravelPlayer 쪽을 우선 진입점으로 사용.
	// PostLogin은 짧은 지연 후 한 번 더 확인만 한다.
	RequestInitializeJoinedPlayer(NewPlayer, PostLoginInitializeDelay);
}

void ATWGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	// 기존 플레이어의 실제 초기화 진입점
	InitializeJoinedPlayer(C);
}

void ATWGameMode::RequestInitializeJoinedPlayer(AController* NewController, float DelaySeconds)
{
	if (!HasAuthority() || !NewController)
	{
		return;
	}

	if (DelaySeconds <= 0.0f)
	{
		InitializeJoinedPlayer(NewController);
		return;
	}

	FTimerDelegate InitDelegate;
	InitDelegate.BindUObject(this, &ATWGameMode::InitializeJoinedPlayer, NewController);

	FTimerHandle InitTimerHandle;
	GetWorldTimerManager().SetTimer(InitTimerHandle, InitDelegate, DelaySeconds, false);
}

void ATWGameMode::InitializeJoinedPlayer(AController* NewController)
{
	APlayerController* PC = Cast<APlayerController>(NewController);
	if (!PC)
	{
		return;
	}

	ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
	if (!PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] InitializeJoinedPlayer skipped: PlayerState is null"));
		return;
	}

	if (HasAlreadyInitializedPlayer(PS))
	{
		return;
	}

	if (PS->PlayerSlot < 0)
	{
		const int32 AssignedSlot = AllocateNextPlayerSlot();
		PS->SetPlayerSlot(AssignedSlot);
		PS->SetTeamID(AssignedSlot);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[GameMode] Player slot assigned | Slot=%d | HeroId=%s"),
			AssignedSlot,
			*PS->GetSelectedHeroUnitId().ToString()
		);
	}

	ATWNexusBuilding* AssignedNexus = PS->GetAssignedStartNexus();
	if (!IsValid(AssignedNexus))
	{
		AssignedNexus = AssignRandomStartNexus(PS);
		if (IsValid(AssignedNexus))
		{
			PS->SetAssignedStartNexus(AssignedNexus);
			AssignNexusOwnership(AssignedNexus, PS);
		}
	}

	if (!IsValid(AssignedNexus))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GameMode] InitializeJoinedPlayer failed: no start nexus available | Slot=%d"),
			PS->PlayerSlot
		);
		return;
	}

	BindPlacedBuildingsForPlayer(PS);

	const bool bAlreadyHasHero = HasSpawnedInitialHeroForPlayer(PS);
	const bool bSpawnedNow = bAlreadyHasHero ? true : SpawnSelectedHeroUnitForPlayer(PS, AssignedNexus);

	if (!bAlreadyHasHero && !bSpawnedNow)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GameMode] Initial hero spawn failed -> scheduling retry | Slot=%d | HeroId=%s"),
			PS->PlayerSlot,
			*PS->GetSelectedHeroUnitId().ToString()
		);

		ScheduleInitialHeroSpawnRetry(PS, InitialHeroSpawnMaxRetryCount);
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[GameMode] Player initialized complete | Slot=%d | Nexus=%s | HeroId=%s | HeroReady=%d"),
		PS->PlayerSlot,
		*GetNameSafe(AssignedNexus),
		*PS->GetSelectedHeroUnitId().ToString(),
		HasSpawnedInitialHeroForPlayer(PS) ? 1 : 0
	);
}

bool ATWGameMode::HasAlreadyInitializedPlayer(const ATWPlayerState* PS) const
{
	if (!PS)
	{
		return false;
	}

	return
		(PS->PlayerSlot >= 0) &&
		IsValid(PS->GetAssignedStartNexus()) &&
		HasSpawnedInitialHeroForPlayer(PS);
}

bool ATWGameMode::HasSpawnedInitialHeroForPlayer(const ATWPlayerState* PS) const
{
	if (!PS || !GetWorld() || PS->PlayerSlot < 0)
	{
		return false;
	}

	for (TActorIterator<ATWHeroUnitBase> It(GetWorld()); It; ++It)
	{
		ATWHeroUnitBase* HeroUnit = *It;
		if (!IsValid(HeroUnit))
		{
			continue;
		}

		if (HeroUnit->GetOwnerPlayerSlot() != PS->PlayerSlot)
		{
			continue;
		}

		if (HeroUnit->IsHeroDead())
		{
			continue;
		}

		return true;
	}

	return false;
}

void ATWGameMode::ScheduleInitialHeroSpawnRetry(ATWPlayerState* PS, int32 RemainingRetryCount)
{
	if (!HasAuthority() || !PS)
	{
		return;
	}

	if (RemainingRetryCount <= 0)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[GameMode] Initial hero spawn retry exhausted | Slot=%d | HeroId=%s"),
			PS->PlayerSlot,
			*PS->GetSelectedHeroUnitId().ToString()
		);
		return;
	}

	if (HasSpawnedInitialHeroForPlayer(PS))
	{
		return;
	}

	FTimerDelegate RetryDelegate;
	RetryDelegate.BindUObject(this, &ATWGameMode::RetryInitialHeroSpawn, PS, RemainingRetryCount);

	FTimerHandle RetryTimerHandle;
	GetWorldTimerManager().SetTimer(RetryTimerHandle, RetryDelegate, InitialHeroSpawnRetryDelay, false);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[GameMode] Initial hero spawn retry scheduled | Slot=%d | Remaining=%d"),
		PS->PlayerSlot,
		RemainingRetryCount
	);
}

void ATWGameMode::RetryInitialHeroSpawn(ATWPlayerState* PS, int32 RemainingRetryCount)
{
	if (!HasAuthority() || !PS)
	{
		return;
	}

	if (HasAlreadyInitializedPlayer(PS))
	{
		return;
	}

	ATWNexusBuilding* Nexus = PS->GetAssignedStartNexus();
	if (!IsValid(Nexus))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GameMode] RetryInitialHeroSpawn skipped: nexus invalid | Slot=%d"),
			PS->PlayerSlot
		);
		return;
	}

	if (HasSpawnedInitialHeroForPlayer(PS))
	{
		return;
	}

	const bool bSpawned = SpawnSelectedHeroUnitForPlayer(PS, Nexus);
	if (bSpawned)
	{
		BindPlacedBuildingsForPlayer(PS);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[GameMode] Initial hero spawn retry success | Slot=%d | HeroId=%s"),
			PS->PlayerSlot,
			*PS->GetSelectedHeroUnitId().ToString()
		);
		return;
	}

	ScheduleInitialHeroSpawnRetry(PS, RemainingRetryCount - 1);
}

int32 ATWGameMode::AllocateNextPlayerSlot() const
{
	TSet<int32> UsedSlots;

	if (!GetWorld())
	{
		return 0;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		const ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
		if (!PS)
		{
			continue;
		}

		if (PS->PlayerSlot >= 0)
		{
			UsedSlots.Add(PS->PlayerSlot);
		}
	}

	int32 CandidateSlot = 0;
	while (UsedSlots.Contains(CandidateSlot))
	{
		++CandidateSlot;
	}

	return CandidateSlot;
}

ATWNexusBuilding* ATWGameMode::AssignRandomStartNexus(ATWPlayerState* PS)
{
	if (!GetWorld() || !PS)
	{
		return nullptr;
	}

	TArray<ATWNexusBuilding*> FreeNexusList;

	for (TActorIterator<ATWNexusBuilding> It(GetWorld()); It; ++It)
	{
		ATWNexusBuilding* Nexus = *It;
		if (!IsValid(Nexus))
		{
			continue;
		}

		if (Nexus->GetOwnerPlayerState() != nullptr)
		{
			continue;
		}

		FreeNexusList.Add(Nexus);
	}

	if (FreeNexusList.Num() <= 0)
	{
		return nullptr;
	}

	const int32 PickIndex = FMath::RandRange(0, FreeNexusList.Num() - 1);
	return FreeNexusList[PickIndex];
}

void ATWGameMode::AssignNexusOwnership(ATWNexusBuilding* Nexus, ATWPlayerState* PS)
{
	if (!IsValid(Nexus) || !PS)
	{
		return;
	}

	Nexus->SetOwnerPlayerSlot(PS->PlayerSlot);
	Nexus->SetOwnerPlayerState(PS);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[GameMode] Nexus ownership assigned | Nexus=%s | Slot=%d"),
		*GetNameSafe(Nexus),
		PS->PlayerSlot
	);
}

FVector ATWGameMode::CalculateAllStartNexusCenter() const
{
	if (!GetWorld())
	{
		return FVector::ZeroVector;
	}

	FVector Accumulated = FVector::ZeroVector;
	int32 Count = 0;

	for (TActorIterator<ATWNexusBuilding> It(GetWorld()); It; ++It)
	{
		const ATWNexusBuilding* Nexus = *It;
		if (!IsValid(Nexus))
		{
			continue;
		}

		Accumulated += Nexus->GetActorLocation();
		++Count;
	}

	if (Count <= 0)
	{
		return FVector::ZeroVector;
	}

	return Accumulated / static_cast<float>(Count);
}

FVector ATWGameMode::CalculateMapInwardDirectionFromNexus(const ATWNexusBuilding* Nexus) const
{
	if (!IsValid(Nexus))
	{
		return FVector::ForwardVector;
	}

	const FVector NexusLocation = Nexus->GetActorLocation();
	FVector NexusCenter = CalculateAllStartNexusCenter();

	// Z는 무시하고 XY 평면 기준으로 계산
	NexusCenter.Z = NexusLocation.Z;

	FVector InwardDirection = NexusCenter - NexusLocation;
	InwardDirection.Z = 0.0f;

	// 혹시 모든 넥서스 중심 계산이 애매하거나 거의 같은 위치면 fallback
	if (InwardDirection.IsNearlyZero())
	{
		InwardDirection = -Nexus->GetActorForwardVector();
		InwardDirection.Z = 0.0f;
	}

	if (InwardDirection.IsNearlyZero())
	{
		InwardDirection = FVector::ForwardVector;
	}

	InwardDirection.Normalize();
	return InwardDirection;
}

FVector ATWGameMode::CalculateHeroSpawnLocationFromNexus(const ATWNexusBuilding* Nexus)
{
	if (!IsValid(Nexus))
	{
		return FVector::ZeroVector;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::ZeroVector;
	}

	FVector Origin = FVector::ZeroVector;
	FVector BoxExtent = FVector::ZeroVector;
	Nexus->GetActorBounds(true, Origin, BoxExtent);

	const FVector InwardDirection = CalculateMapInwardDirectionFromNexus(Nexus);

	const float InwardExtent =
		FMath::Abs(InwardDirection.X) * BoxExtent.X +
		FMath::Abs(InwardDirection.Y) * BoxExtent.Y;

	const float SpawnOffsetDistance =
		InwardExtent +
		StartHeroSpawnDistance +
		StartHeroSpawnExtraPadding;

	FVector SpawnLocation = Origin + (InwardDirection * SpawnOffsetDistance);

	FVector RightDirection = FVector::CrossProduct(FVector::UpVector, InwardDirection);
	RightDirection.Z = 0.0f;

	if (!RightDirection.IsNearlyZero())
	{
		RightDirection.Normalize();

		const float LateralOffset = FMath::FRandRange(
			-StartHeroLateralRandomOffset,
			StartHeroLateralRandomOffset
		);

		SpawnLocation += RightDirection * LateralOffset;
	}

	// 1차: NavMesh 투영
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation ProjectedLocation;
		const FVector QueryExtent(
			StartHeroNavProjectExtentXY,
			StartHeroNavProjectExtentXY,
			StartHeroNavProjectExtentZ
		);

		if (NavSys->ProjectPointToNavigation(SpawnLocation, ProjectedLocation, QueryExtent))
		{
			SpawnLocation = ProjectedLocation.Location;
		}
	}

	{
		const FVector TraceStart = SpawnLocation + FVector(0.0f, 0.0f, StartHeroGroundTraceHalfHeight);
		const FVector TraceEnd   = SpawnLocation - FVector(0.0f, 0.0f, StartHeroGroundTraceHalfHeight);

		FHitResult HitResult;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HeroSpawnGroundTrace), false);
		QueryParams.AddIgnoredActor(Nexus);

		if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			SpawnLocation.Z = HitResult.ImpactPoint.Z + 6.0f;
		}
	}

	return SpawnLocation;
}

FRotator ATWGameMode::CalculateHeroSpawnRotationFromNexus(const ATWNexusBuilding* Nexus)
{
	if (!IsValid(Nexus))
	{
		return FRotator::ZeroRotator;
	}

	const FVector SpawnLocation = CalculateHeroSpawnLocationFromNexus(Nexus);

	FVector LookDirection = Nexus->GetActorLocation() - SpawnLocation;
	LookDirection.Z = 0.0f;

	if (LookDirection.IsNearlyZero())
	{
		LookDirection = -Nexus->GetActorForwardVector();
		LookDirection.Z = 0.0f;
	}

	LookDirection.Normalize();

	FRotator ResultRotation = LookDirection.Rotation();
	ResultRotation.Yaw += StartHeroYawOffset;
	ResultRotation.Pitch = 0.0f;
	ResultRotation.Roll = 0.0f;

	return ResultRotation;
}

bool ATWGameMode::SpawnSelectedHeroUnitForPlayer(ATWPlayerState* PS, ATWNexusBuilding* Nexus)
{
	if (!HasAuthority() || !PS || !IsValid(Nexus))
	{
		return false;
	}

	if (PS->GetSelectedHeroUnitId().IsNone())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GameMode] SpawnSelectedHeroUnitForPlayer failed: HeroId is None | Slot=%d"),
			PS->PlayerSlot
		);
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UTWUnitSubsystem* UnitSubsystem = World->GetSubsystem<UTWUnitSubsystem>();
	if (!UnitSubsystem)
	{
		return false;
	}

	ATWPlayerController* OwnerPC = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		if (PC->GetPlayerState<ATWPlayerState>() == PS)
		{
			OwnerPC = Cast<ATWPlayerController>(PC);
			break;
		}
	}

	if (!OwnerPC)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GameMode] SpawnSelectedHeroUnitForPlayer failed: OwnerPC not found | Slot=%d"),
			PS->PlayerSlot
		);
		return false;
	}

	const FVector SpawnLocation = CalculateHeroSpawnLocationFromNexus(Nexus);
	const FRotator SpawnRotation = CalculateHeroSpawnRotationFromNexus(Nexus);

	const bool bSpawned = UnitSubsystem->SpawnHeroUnitById(
		SpawnLocation,
		PS->GetSelectedHeroUnitId(),
		OwnerPC
	);

	if (bSpawned)
	{
		for (TActorIterator<ATWHeroUnitBase> It(World); It; ++It)
		{
			ATWHeroUnitBase* HeroUnit = *It;
			if (!IsValid(HeroUnit))
			{
				continue;
			}

			if (HeroUnit->GetOwnerPlayerSlot() != PS->PlayerSlot)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(HeroUnit->GetActorLocation(), SpawnLocation);
			if (DistSq > 300.0f * 300.0f)
			{
				continue;
			}

			HeroUnit->SetActorLocation(SpawnLocation);
			HeroUnit->SetActorRotation(SpawnRotation);

			break;
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[GameMode] Hero spawn request | Slot=%d | HeroId=%s | Success=%d | SpawnLocation=%s | SpawnYaw=%.2f"),
		PS->PlayerSlot,
		*PS->GetSelectedHeroUnitId().ToString(),
		bSpawned ? 1 : 0,
		*SpawnLocation.ToString(),
		SpawnRotation.Yaw
	);

	return bSpawned;
}

void ATWGameMode::RequestRespawnHero(ATWPlayerState* PS, FName HeroUnitId, float DelaySeconds)
{
	if (!HasAuthority() || !PS || HeroUnitId.IsNone())
	{
		return;
	}

	if (PS->IsHeroRespawnPending() == false)
	{
		PS->SetHeroRespawnPending(true);
	}

	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(this, &ATWGameMode::RespawnHero, PS, HeroUnitId);

	FTimerHandle RespawnTimerHandle;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, DelaySeconds, false);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[GameMode] Hero respawn scheduled | Slot=%d | HeroId=%s | Delay=%.2f"),
		PS->PlayerSlot,
		*HeroUnitId.ToString(),
		DelaySeconds
	);
}

void ATWGameMode::RespawnHero(ATWPlayerState* PS, FName HeroUnitId)
{
	if (!HasAuthority() || !PS)
	{
		return;
	}

	if (HasSpawnedInitialHeroForPlayer(PS))
	{
		PS->SetHeroRespawnPending(false);
		return;
	}

	ATWNexusBuilding* Nexus = PS->GetAssignedStartNexus();
	if (!IsValid(Nexus))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GameMode] RespawnHero failed: invalid nexus | Slot=%d | HeroId=%s"),
			PS->PlayerSlot,
			*HeroUnitId.ToString()
		);

		PS->SetHeroRespawnPending(false);
		return;
	}

	const bool bSpawned = SpawnSelectedHeroUnitForPlayer(PS, Nexus);
	if (bSpawned)
	{
		PS->SetHeroRespawnPending(false);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[GameMode] RespawnHero success | Slot=%d | HeroId=%s"),
			PS->PlayerSlot,
			*HeroUnitId.ToString()
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[GameMode] RespawnHero failed | Slot=%d | HeroId=%s"),
			PS->PlayerSlot,
			*HeroUnitId.ToString()
		);
	}
}
ATWPlayerState* ATWGameMode::FindPlayerStateBySlot(int32 InPlayerSlot) const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
		if (!PS)
		{
			continue;
		}

		if (PS->PlayerSlot == InPlayerSlot)
		{
			return PS;
		}
	}

	return nullptr;
}

void ATWGameMode::TryBindBuilding(ATWBaseBuilding* InBuilding)
{
	if (!InBuilding)
	{
		return;
	}

	if (InBuilding->GetOwnerPlayerState())
	{
		return;
	}

	ATWPlayerState* FoundPlayerState = FindPlayerStateBySlot(InBuilding->GetOwnerPlayerSlot());
	if (!FoundPlayerState)
	{
		return;
	}

	InBuilding->SetOwnerPlayerState(FoundPlayerState);
}

void ATWGameMode::HandlePlayerDefeat(int32 DefeatedPlayerSlot)
{
	const int32 CurrentPlayerCount = GameState ? GameState->PlayerArray.Num() : 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		int32 FinalResult = -1;

		ATWPlayerController* PC = Cast<ATWPlayerController>(It->Get());
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
		if (!PS)
		{
			continue;
		}

		if (CurrentPlayerCount <= 1)
		{
			FinalResult = 1;
		}
		else if (CurrentPlayerCount == 2)
		{
			FinalResult = (PS->PlayerSlot == DefeatedPlayerSlot) ? 0 : 1;
		}
		else
		{
			if (PS->PlayerSlot == DefeatedPlayerSlot)
			{
				FinalResult = 0;
			}
			else
			{
				continue;
			}
		}

		if (FinalResult != -1)
		{
			PS->SetGameResult(FinalResult);
			PC->Client_ShowGameResult(FinalResult);
		}
	}
}

void ATWGameMode::BindPlacedBuildingsForPlayer(ATWPlayerState* InPlayerState)
{
	if (!InPlayerState)
	{
		return;
	}

	for (TActorIterator<ATWBaseBuilding> It(GetWorld()); It; ++It)
	{
		ATWBaseBuilding* Building = *It;
		if (!Building)
		{
			continue;
		}

		if (Building->GetOwnerPlayerState())
		{
			continue;
		}

		if (Building->GetOwnerPlayerSlot() != InPlayerState->PlayerSlot)
		{
			continue;
		}

		Building->SetOwnerPlayerState(InPlayerState);
	}
}