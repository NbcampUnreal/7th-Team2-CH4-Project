#include "FOW/Test_MyGameMode.h"
#include "FOW/Test_MyPlayerState.h"
#include "FOW/TW_TestCharacter.h"
#include "GameFramework/PlayerController.h"

void ATest_MyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (!NewPlayer) return;

	ATest_MyPlayerState* PS = NewPlayer->GetPlayerState<ATest_MyPlayerState>();
	if (PS)
	{
		PlayerCount++;
		int32 UniqueTeamID = PlayerCount;
		PS->TeamID = UniqueTeamID; //[cite: 7]

		// [추가] 서버에서 이미 빙의된 캐릭터가 있다면 즉시 동기화 강제 수행
		if (ATW_TestCharacter* TestChar = Cast<ATW_TestCharacter>(NewPlayer->GetPawn()))
		{
			TestChar->SyncTeamFromPlayerState();
		}
        
		UE_LOG(LogTemp, Warning, TEXT("=== SERVER: Player [%s] Team ID Assigned: %d ==="), 
			   *NewPlayer->GetName(), PS->TeamID);
	}
}
