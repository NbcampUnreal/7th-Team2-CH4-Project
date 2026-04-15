#include "Building/TWBaseBuilding.h"
#include "Core/TWPlayerState.h"
#include "Data/TWBuildingDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/TWGridSubSystem.h"

ATWBaseBuilding::ATWBaseBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	bReplicates = true;
	bHasCachedMaterials = false;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
}

void ATWBaseBuilding::BeginPlay()
{
	Super::BeginPlay();
	
	CacheOriginalMaterial();
	UpdatePlayerMaterial();
	
		
	if (UWorld* World = GetWorld())
	{
		if (UTWGridSubSystem* GridSub = World->GetSubsystem<UTWGridSubSystem>())
		{
			FIntPoint Anchor = GridSub->WorldToGridPosition(GetActorLocation());
			FIntPoint Size = BuildingData ? BuildingData->GridSize.BuildingSize : FIntPoint(1,1);
			GridSub->OccupyArea(Anchor, Size, this);
		}
	}
	
	if (!HasAuthority())
	{
		return;
	}

	if (!BuildingData)
	{
		return;
	}

	MaxHP = FMath::Max(1.0f, BuildingData->MaxHP);
	CurrentHP = MaxHP;
	MaxBuildTime = BuildingData->BuildTime;
	
	if (MaxBuildTime > 0.0f)
	{
		StartConstruction();
	}
	else
	{
		CurrentHP = MaxHP;
		BuildingState = ETWBuildingState::Completed;
		OnRep_BuildingState();
	}
}

void ATWBaseBuilding::Destroyed()
{
	ClearAllBuildingTimers();
	
	if (UWorld* World = GetWorld())
	{
		if (UTWGridSubSystem* GridSub = World->GetSubsystem<UTWGridSubSystem>())
		{
			FIntPoint Anchor = GridSub->WorldToGridPosition(GetActorLocation());
			FIntPoint Size = BuildingData ? BuildingData->GridSize.BuildingSize : FIntPoint(1,1);
			
			GridSub->FreeArea(Anchor, Size);
		}
	}
	Super::Destroyed();
}

void ATWBaseBuilding::ApplyDamageToBuilding(const float InDamageAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InDamageAmount <= 0.0f)
	{
		return;
	}

	if (CurrentHP <= 0.0f)
	{
		return;
	}

	CurrentHP -= InDamageAmount;
	CurrentHP = FMath::Max(0.0f, CurrentHP);

	UE_LOG(LogTemp, Warning, TEXT("[%s] HP : %.1f / %.1f"), *GetName(), CurrentHP, MaxHP);

	if (CurrentHP <= 0.0f)
	{
		HandleDestroyedByDamage();
	}
}

float ATWBaseBuilding::GetBuildingProgress() const
{
	if (MaxBuildTime <= 0.0f)
	{
		return 1.0f;
	}
	return FMath::Clamp(CurrentBuildTime / MaxBuildTime, 0.0f, 1.0f);
}

float ATWBaseBuilding::GetRemainingBuildTime() const
{
	return FMath::Max(0.0f, MaxBuildTime - CurrentBuildTime);
}

void ATWBaseBuilding::HandleDestroyedByDamage()
{
	if (!HasAuthority())
	{
		return;
	}

	Destroy();
}

void ATWBaseBuilding::OnRep_OwnerPlayerSlot()
{
	UpdatePlayerMaterial();
}

void ATWBaseBuilding::OnRep_BuildingState()
{
	if (!MeshComponent)
	{
		return;
	}
	
	if (BuildingState == ETWBuildingState::UnderConstruction)
	{
		if (ConstructionMaterial)
		{
			for (int32 i = 0; i < MeshComponent->GetNumMaterials(); ++i)
			{
				MeshComponent->SetMaterial(i, ConstructionMaterial);
			}
		}
	}
	else if (BuildingState == ETWBuildingState::Completed)
	{
		for (int32 i = 0; i < MeshComponent->GetNumMaterials(); ++i)
		{
			if (OriginalMaterials.IsValidIndex(i))
			{
				MeshComponent->SetMaterial(i, OriginalMaterials[i]);	
			}
		}
	}
}

void ATWBaseBuilding::CacheOriginalMaterial()
{
	if (bHasCachedMaterials || !MeshComponent)
	{
		return;
	}
	
	const int32 NumMats = MeshComponent->GetNumMaterials();
	for (int32 i = 0; i < NumMats; ++i)
	{
		OriginalMaterials.Add(MeshComponent->GetMaterial(i));
	}
	
	bHasCachedMaterials = true;
}

void ATWBaseBuilding::UpdatePlayerMaterial()
{
	CacheOriginalMaterial();
	
	if (PlayerMaterialMap.Contains(OwnerPlayerSlot))
	{
		UMaterialInterface* PlayerMat = PlayerMaterialMap[OwnerPlayerSlot];
		
		if (PlayerMat && OriginalMaterials.IsValidIndex(PlayerMaterialIndex))
		{
			OriginalMaterials[PlayerMaterialIndex] = PlayerMat;
			
			if (BuildingState == ETWBuildingState::Completed && MeshComponent)
			{
				MeshComponent->SetMaterial(PlayerMaterialIndex, PlayerMat);
			}
		}
	}
}

void ATWBaseBuilding::StartConstruction()
{
	BuildingState = ETWBuildingState::UnderConstruction;
	CurrentBuildTime = 0.0f;
	CurrentHP = 1.0f;
	
	OnRep_BuildingState();
	
	GetWorld()->GetTimerManager().SetTimer(
		ConstructionTimerHandle,
		this,
		&ATWBaseBuilding::UpdateConstruction,
		ConstructionTickInterval,
		true
		);
}

void ATWBaseBuilding::UpdateConstruction()
{
	CurrentBuildTime += ConstructionTickInterval;
	
	float HPGainPerTick = (MaxHP / MaxBuildTime) * ConstructionTickInterval;
	CurrentHP = FMath::Clamp(CurrentHP + HPGainPerTick, 0.0f, MaxHP);
	
	UE_LOG(LogTemp, Warning, TEXT("RemainBuildTime : %.2f"), CurrentBuildTime);
	
	if (CurrentBuildTime >= MaxBuildTime)
	{
		FinishConstruction();
	}
}

void ATWBaseBuilding::FinishConstruction()
{
	BuildingState = ETWBuildingState::Completed;
	CurrentHP = MaxHP;
	
	GetWorld()->GetTimerManager().ClearTimer(ConstructionTimerHandle);
	UE_LOG(LogTemp, Warning, TEXT("BuildCompleted"));
	OnRep_BuildingState();
}

void ATWBaseBuilding::SetOwnerPlayerSlot(int32 InSlot)
{
	if (HasAuthority())
	{
		OwnerPlayerSlot = InSlot;
		
		UpdatePlayerMaterial();
	}
}

void ATWBaseBuilding::SetOwnerPlayerState(ATWPlayerState* InPlayerState)
{
	if (!HasAuthority())
	{
		return;
	}
	
	OwningPlayerState = InPlayerState;
	OnOwnerPlayerStateAssigned();
}

void ATWBaseBuilding::OnOwnerPlayerStateAssigned()
{
}

void ATWBaseBuilding::ClearAllBuildingTimers()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ConstructionTimerHandle);
	}
}

void ATWBaseBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWBaseBuilding, OwnerPlayerSlot);
	DOREPLIFETIME(ATWBaseBuilding, OwningPlayerState);
	DOREPLIFETIME(ATWBaseBuilding, MaxHP);
	DOREPLIFETIME(ATWBaseBuilding, CurrentHP);
	DOREPLIFETIME(ATWBaseBuilding, BuildingState);
}