#include "Component/TW_TeamComponent.h"

#include "Net/UnrealNetwork.h"

UTW_TeamComponent::UTW_TeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UTW_TeamComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UTW_TeamComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTW_TeamComponent, TeamID);
}

void UTW_TeamComponent::OnRep_TeamID()
{
	if (OnTeamChanged.IsBound())
	{
		OnTeamChanged.Broadcast(TeamID);
	}
}


void UTW_TeamComponent::SetTeamID(int32 NewTeamID)
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
