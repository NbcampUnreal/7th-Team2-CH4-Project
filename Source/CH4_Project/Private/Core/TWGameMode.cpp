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

ATWPlayerState* ATWGameMode::FindPlayerStateBySlot(int32 InPlayerSlot) const
{
	if (!GetWorld())
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
		if (!PS)
		{
			continue;
		}

		if (PS->PlayerSlot == InPlayerSlot)
		{
			return PS;
		}
	}

	return nullptr;
}

void ATWGameMode::TryBindBuilding(ATWBaseBuilding* InBuilding)
{
	if (!InBuilding)
	{
		return;
	}

	if (InBuilding->GetOwnerPlayerState())
	{
		return;
	}

	ATWPlayerState* FoundPlayerState = FindPlayerStateBySlot(InBuilding->GetOwnerPlayerSlot());
	if (!FoundPlayerState)
	{
		return;
	}

	InBuilding->SetOwnerPlayerState(FoundPlayerState);
}

void ATWGameMode::HandlePlayerDefeat(int32 DefeatedPlayerSlot)
{
	int32 WinnerPlayerSlot = -1;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
		{
			continue;
		}

		ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
		if (!PS)
		{
			continue;
		}

		if (PS->PlayerSlot != DefeatedPlayerSlot)
		{
			WinnerPlayerSlot = PS->PlayerSlot;
			break;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Player %d Defeated / Player %d Victory"), DefeatedPlayerSlot, WinnerPlayerSlot);
	
	// 1) 입력 막기
	// 2) 승패 UI 표시
	// 3) MatchState 변경
	// 4) 리스타트/결과 화면 이동
}

// 미리 설치되어 있는 건물 바인딩
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
