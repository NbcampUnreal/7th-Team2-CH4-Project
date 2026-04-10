#include "Core/TWGameMode.h"
#include "Core/TWPlayerState.h"
#include "Building/TWResourceBuilding.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void ATWGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	ATWPlayerState* PS = NewPlayer->GetPlayerState<ATWPlayerState>();
	if (!PS)
	{
		return;
	}

	// 접속 순서대로 0, 1 ... 부여
	int32 AssignedSlot = 0;

	int32 ExistingAssignedCount = 0; // 이미 슬롯이 배정된 다른 플레이어 수 (나 제외)
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* OtherPS = PC->GetPlayerState<ATWPlayerState>();
		if (!OtherPS)
		{
			continue;
		}

		if (OtherPS != PS && OtherPS->PlayerSlot != -1)
		{
			ExistingAssignedCount++;
		}
	}

	AssignedSlot = ExistingAssignedCount;

	PS->SetPlayerSlot(AssignedSlot);
	
	BindPlacedBuildingsForPlayer(PS);
}

// 테스트를 위한 함수
void ATWGameMode::BindPlacedBuildingsForPlayer(ATWPlayerState* InPlayerState)
{
	if (!InPlayerState)
	{
		return;
	}

	for (TActorIterator<ATWBaseBuilding> It(GetWorld()); It; ++It)
	{
		ATWBaseBuilding* Building = *It;
		if (!Building)
		{
			continue;
		}

		if (Building->GetOwnerPlayerState())
		{
			continue;
		}

		if (Building->OwnerPlayerSlot != InPlayerState->PlayerSlot)
		{
			continue;
		}

		Building->SetOwnerPlayerState(InPlayerState);
	}
}