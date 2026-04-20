#include "Building/TWBaseBuilding.h"

#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Data/TWBuildingDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/TWBuildingManagerSubsystem.h"
#include "Subsystems/TWGridSubSystem.h"
#include "NavModifierComponent.h"
#include "Component/TWTeamColorComponent.h"
#include "Component/TWTeamComponent.h"
#include "FOW/TWVisionComponent.h"
#include "NavAreas/NavArea_Null.h"

ATWBaseBuilding::ATWBaseBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	
	NavModifier = CreateDefaultSubobject<UNavModifierComponent>(TEXT("NavModifier"));
	NavModifier->SetAreaClass(UNavArea_Null::StaticClass());
	
	MeshComponent->SetCanEverAffectNavigation(false);
	
	TeamColorComponent = CreateDefaultSubobject<UTWTeamColorComponent>(TEXT("TeamColorComponent"));
	TeamComponent = CreateDefaultSubobject<UTWTeamComponent>(TEXT("TeamComponent"));
	FogVisionComponent = CreateDefaultSubobject<UTWVisionComponent>(TEXT("FogVisionComponent"));
	if (FogVisionComponent)
	{
		FogVisionComponent->VisionRadius = 2000.f;
	}
}

void ATWBaseBuilding::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (TeamColorComponent && MeshComponent)
	{
		TeamColorComponent->SetUpTargetMesh(MeshComponent);
	}
}

void ATWBaseBuilding::BeginPlay()
{
	Super::BeginPlay();
	
	if (UWorld* World = GetWorld())
	{
		if (UTWGridSubSystem* GridSub = World->GetSubsystem<UTWGridSubSystem>())
		{
			FIntPoint Anchor = GridSub->WorldToGridPosition(GetActorLocation());
			FIntPoint Size = BuildingData ? BuildingData->GridSize.BuildingSize : FIntPoint(1,1);
			GridSub->OccupyArea(Anchor, Size, this);
		}
	}
	
	if (UWorld* World = GetWorld())
	{
		if (auto* BuildingManager = World->GetSubsystem<UTWBuildingManagerSubsystem>())
		{
			BuildingManager->RegisterBuilding(OwnerPlayerSlot, this);
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
	
	if (UWorld* World = GetWorld())
	{
		if (auto* BuildingManager = World->GetSubsystem<UTWBuildingManagerSubsystem>())
		{
			BuildingManager->UnregisterBuilding(OwnerPlayerSlot, this);
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

	NotifyRecentCombatBuildingDamaged(1.25f);
	
	if (CurrentHP <= 0.0f)
	{
		HandleDestroyedByDamage();
	}
}

void ATWBaseBuilding::NotifyRecentCombatBuildingDamaged(float InVisibleTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SafeVisibleTime = (InVisibleTime > 0.0f) ? InVisibleTime : 1.25f;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ATWPlayerController* TargetPC = Cast<ATWPlayerController>(It->Get());
		if (!TargetPC)
		{
			continue;
		}

		TargetPC->ClientNotifyRecentCombatBuildingDamaged(this, SafeVisibleTime);
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

void ATWBaseBuilding::SetSelectionVisualActive(bool bInActive)
{
	bSelectionVisualActive = bInActive;
}

FVector ATWBaseBuilding::GetSelectionAnchorWorldLocation() const
{
	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);

	const FVector BaseLocation(
		Origin.X,
		Origin.Y,
		Origin.Z - Extent.Z
	);

	return BaseLocation + SelectionAnchorOffset;
}

FVector ATWBaseBuilding::GetHPBarAnchorWorldLocation() const
{
	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);

	const FVector TopLocation(
		Origin.X,
		Origin.Y,
		Origin.Z + Extent.Z
	);

	return TopLocation + HPBarAnchorOffset;
}

float ATWBaseBuilding::GetSelectionVisualRadius() const
{
	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);

	return FMath::Max(Extent.X, Extent.Y);
}

FVector2D ATWBaseBuilding::GetSelectionRectangleHalfExtentXY(float InPadding) const
{
	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);

	return FVector2D(
		FMath::Max(0.f, Extent.X + InPadding),
		FMath::Max(0.f, Extent.Y + InPadding)
	);
}

float ATWBaseBuilding::GetSelectionRectangleZOffset(float InBaseOffset) const
{
	FVector Origin = FVector::ZeroVector;
	FVector Extent = FVector::ZeroVector;
	GetActorBounds(false, Origin, Extent);

	return Extent.Z + InBaseOffset;
}

FVector ATWBaseBuilding::GetSelectionHPBarWorldLocation() const
{
	return GetHPBarAnchorWorldLocation();
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
	if (TeamColorComponent)
	{
		TeamColorComponent->ApplyTeamColor(OwnerPlayerSlot);
		
		if (BuildingState == ETWBuildingState::UnderConstruction)
		{
			OnRep_BuildingState();
		}
	}
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
		if (TeamColorComponent)
		{
			TeamColorComponent->RestoreOriginalMaterials();
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
	
	const float HPGainPerTick = (MaxHP / MaxBuildTime) * ConstructionTickInterval;
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
		
		if (TeamColorComponent)
		{
			TeamColorComponent->ApplyTeamColor(OwnerPlayerSlot);
			
			if (BuildingState == ETWBuildingState::UnderConstruction)
			{
				OnRep_BuildingState();
			}
		}
		
		if (TeamComponent)
		{
			TeamComponent->SetTeamID(OwnerPlayerSlot);
		}
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
