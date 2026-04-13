#include "Component/TWTeamComponent.h"

#include "Net/UnrealNetwork.h"

UTWTeamComponent::UTWTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTWTeamComponent::BeginPlay()
{
	Super::BeginPlay();
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
