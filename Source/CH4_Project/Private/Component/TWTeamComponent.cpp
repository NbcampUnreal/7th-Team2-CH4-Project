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

}

void UTWTeamComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsTeamInitialized || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	
	UMassAgentComponent* MassAgent = GetOwner()->FindComponentByClass<UMassAgentComponent>();
	if (MassAgent && MassAgent->GetEntityHandle().IsValid())
	{
		GetTeamID();
		
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
	
	if (false == GetOwner()->HasAuthority())
	{
		return TeamID;
	}
	UMassAgentComponent* MassAgentComponent = GetOwner()->FindComponentByClass<UMassAgentComponent>();
	if (false == IsValid(MassAgentComponent))
	{
		return TeamID;
	}
	FMassEntityManager* MassEntityManager = UE::Mass::Utils::GetEntityManager(this);
	if (!MassEntityManager)
	{
		return TeamID;
	}
	int32 NewTeamID = MassEntityManager->GetFragmentDataPtr<FTWUnitFragment>(MassAgentComponent->GetEntityHandle())->GetOwner();
	SetTeamID(NewTeamID);
	
	return NewTeamID;
}


void UTWTeamComponent::SetTeamID(int32 NewTeamID)
{
	// 서버에서만 실행되도록 보장
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	if (TeamID != NewTeamID)
	{
		TeamID = NewTeamID;
		// 서버에서는 OnRep이 자동으로 호출되지 않으므로 수동 호출
		OnRep_TeamID(); 
	}
}
