#include "Building/TWBaseBuilding.h"
#include "Core/TWPlayerState.h"
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

void ATWBaseBuilding::Destroyed()
{
	Super::Destroyed();

	if (!HasAuthority())
	{
		return;
	}

	ClearAllBuildingTimers();
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
}