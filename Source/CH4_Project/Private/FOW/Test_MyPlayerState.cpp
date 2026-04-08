#include "FOW/Test_MyPlayerState.h"

#include "FOW/TW_TestCharacter.h"
#include "Net/UnrealNetwork.h"


ATest_MyPlayerState::ATest_MyPlayerState()
{
	
}

void ATest_MyPlayerState::OnRep_TeamID()
{
	UE_LOG(LogTemp, Warning, TEXT("=== CLIENT: TeamID Replicated: %d ==="), TeamID);
    
	if (APawn* MyPawn = GetPawn())
	{
		if (ATW_TestCharacter* TestChar = Cast<ATW_TestCharacter>(MyPawn))
		{
			TestChar->SyncTeamFromPlayerState();
		}
	}
}

void ATest_MyPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ATest_MyPlayerState, TeamID);
}
