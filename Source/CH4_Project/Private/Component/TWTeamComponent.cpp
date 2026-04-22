#include "Component/TWTeamComponent.h"

#include "MassAgentComponent.h"
#include "Mass/Fragments/TWUnitFragment.h"
#include "Net/UnrealNetwork.h"

UTWTeamComponent::UTWTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
	bIsTeamInitialized = false;
}

void UTWTeamComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(false);
	}
}

void UTWTeamComponent::TickComponent(
	float DeltaTime,
	enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsTeamInitialized || !GetOwner() || !GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(false);
		return;
	}

	UMassAgentComponent* MassAgent = GetOwner()->FindComponentByClass<UMassAgentComponent>();
	if (!MassAgent || !MassAgent->GetEntityHandle().IsValid())
	{
		return;
	}

	GetTeamID();
	if (TeamID >= 0)
	{
		bIsTeamInitialized = true;
		SetComponentTickEnabled(false);
	}
}

void UTWTeamComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTWTeamComponent, TeamID);
}

void UTWTeamComponent::OnRep_TeamID()
{
	if (OnTeamChanged.IsBound())
	{
		OnTeamChanged.Broadcast(TeamID);
	}
}

int32 UTWTeamComponent::GetTeamID()
{
	if (TeamID != -1)
	{
		return TeamID;
	}

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return TeamID;
	}

	UMassAgentComponent* MassAgentComponent = GetOwner()->FindComponentByClass<UMassAgentComponent>();
	if (!IsValid(MassAgentComponent) || !MassAgentComponent->GetEntityHandle().IsValid())
	{
		return TeamID;
	}

	FMassEntityManager* MassEntityManager = UE::Mass::Utils::GetEntityManager(this);
	if (!MassEntityManager)
	{
		return TeamID;
	}

	const FTWUnitFragment* UnitFragment = MassEntityManager->GetFragmentDataPtr<FTWUnitFragment>(MassAgentComponent->GetEntityHandle());
	if (!UnitFragment)
	{
		return TeamID;
	}

	SetTeamID(UnitFragment->GetOwner());
	return TeamID;
}

void UTWTeamComponent::SetTeamID(int32 NewTeamID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (TeamID == NewTeamID)
	{
		return;
	}

	TeamID = NewTeamID;
	OnRep_TeamID();
}
