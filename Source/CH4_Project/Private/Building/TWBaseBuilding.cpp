#include "Building/TWBaseBuilding.h"
#include "Core/TWPlayerState.h"
#include "Data/TWBuildingDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ATWBaseBuilding::ATWBaseBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
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
	
	if (!HasAuthority())
	{
		return;
	}

	if (!BuildingData)
	{
		return;
	}

	MaxHP = FMath::Max(1, BuildingData->MaxHP);
	CurrentHP = MaxHP;
}

void ATWBaseBuilding::Destroyed()
{
	Super::Destroyed();

	if (!HasAuthority())
	{
		return;
	}

	ClearAllBuildingTimers();
}

void ATWBaseBuilding::ApplyDamageToBuilding(const int32 InDamageAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	if (InDamageAmount <= 0)
	{
		return;
	}

	if (CurrentHP <= 0)
	{
		return;
	}

	CurrentHP -= InDamageAmount;
	CurrentHP = FMath::Max(0, CurrentHP);

	UE_LOG(LogTemp, Warning, TEXT("[%s] HP : %d / %d"), *GetName(), CurrentHP, MaxHP);

	if (CurrentHP == 0)
	{
		HandleDestroyedByDamage();
	}
}

void ATWBaseBuilding::HandleDestroyedByDamage()
{
	if (!HasAuthority())
	{
		return;
	}

	Destroy();
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
}

void ATWBaseBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATWBaseBuilding, OwnerPlayerSlot);
	DOREPLIFETIME(ATWBaseBuilding, OwningPlayerState);
	DOREPLIFETIME(ATWBaseBuilding, MaxHP);
	DOREPLIFETIME(ATWBaseBuilding, CurrentHP);
}