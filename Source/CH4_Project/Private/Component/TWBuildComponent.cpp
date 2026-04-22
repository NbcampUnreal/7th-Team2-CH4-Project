#include "Component/TWBuildComponent.h"
#include "GameFramework/PlayerController.h"
#include "Building/TWBaseBuilding.h"
#include "Building/GhostBuilding.h"
#include "Building/TWUpgradeBuilding.h"
#include "Data/TWBuildingDataAsset.h"
#include "Subsystems/TWGridSubSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TWPlayerState.h"
#include "Core/TWGameMode.h"
#include "EngineUtils.h"
#include "Log/TWLogCategory.h"

namespace
{
	static bool HasOwnedUpgradeBuilding(UWorld* World, int32 InPlayerSlot)
	{
		if (!World)
		{
			return false;
		}

		for (TActorIterator<ATWUpgradeBuilding> It(World); It; ++It)
		{
			ATWUpgradeBuilding* UpgradeBuilding = *It;
			if (!UpgradeBuilding)
			{
				continue;
			}

			if (UpgradeBuilding->OwnerPlayerSlot != InPlayerSlot)
			{
				continue;
			}

			return true;
		}

		return false;
	}
}

UTWBuildComponent::UTWBuildComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
	
	bIsBuildMode = false;
}

void UTWBuildComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsBuildMode && CurrentGhost)
	{
		APlayerController* PC = Cast<APlayerController>(GetOwner());
		
		if (PC)
		{
			FHitResult Hit;
			
			PC->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel1), true, Hit);
			
			if (Hit.bBlockingHit)
			{
				auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();
			
				if (GridSub)
				{
					CurrentAnchor = GridSub->WorldToGridPosition(Hit.Location);
			
					bool bCanBuild = GridSub->CanBuildArea(CurrentAnchor, CurrentGhost->BuildingSize);
					CurrentGhost->UpdateBuildingVisual(bCanBuild);
			
					FVector CenterPos = GridSub->GetBuildingCenterPosition(CurrentAnchor, CurrentGhost->BuildingSize);
					CurrentGhost->SetActorLocation(CenterPos);
			
				}
			}
		}
	}
}

void UTWBuildComponent::SelectBuildingToConstruct(EBuildingCategory Category)
{
	if (!BuildingMap.Contains(Category))
	{
		return;
	}
	
	TSubclassOf<ATWBaseBuilding> TargetClass = BuildingMap[Category];
	
	if (bIsBuildMode && SelectedBuildingClass ==  TargetClass)
	{
		EndBuildMode();
		return;
	}
	
	SelectedBuildingClass = TargetClass;
	ATWBaseBuilding* DefaultBuilding = SelectedBuildingClass->GetDefaultObject<ATWBaseBuilding>();
	
	if (!DefaultBuilding || !DefaultBuilding->BuildingData || !DefaultBuilding->MeshComponent)
	{
		return;
	}
	
	if (!bIsBuildMode)
	{
		if (!BuildClass)
		{
			return;
		}
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		CurrentGhost = GetWorld()->SpawnActor<AGhostBuilding>(BuildClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		bIsBuildMode = true;
	}
	
	if (CurrentGhost)
	{
		UStaticMesh* RealMesh = DefaultBuilding->MeshComponent->GetStaticMesh();
		FIntPoint RealSize = DefaultBuilding->BuildingData->GridSize.BuildingSize;
		
		CurrentGhost->SetBuildingMesh(RealMesh, RealSize);
	}
}

void UTWBuildComponent::RequestBuild()
{
	if (!bIsBuildMode || !CurrentGhost || !SelectedBuildingClass)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	ATWPlayerState* TWPS = PC->GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}

	ATWBaseBuilding* DefaultBuilding = SelectedBuildingClass->GetDefaultObject<ATWBaseBuilding>();
	if (!DefaultBuilding || !DefaultBuilding->BuildingData)
	{
		return;
	}
	
	if (DefaultBuilding->IsA(ATWUpgradeBuilding::StaticClass()))
	{
		if (HasOwnedUpgradeBuilding(GetWorld(), TWPS->PlayerSlot))
		{
			UE_LOG(LogTWBuild, Warning, TEXT("[건설] 설치 실패: 연구소는 1개만 건설할 수 있습니다."));
			return;
		}
	}

	auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();
	if (!GridSub)
	{
		return;
	}
	
	if (!GridSub->CanBuildArea(CurrentAnchor, CurrentGhost->BuildingSize))
	{
		UE_LOG(LogTWBuild, Warning, TEXT("[건설] 설치 실패: 배치 불가 위치"));
		return;
	}
	
	if (TWPS->CanAffordCost(DefaultBuilding->BuildingData->BuildCost) == 0)
	{
		UE_LOG(LogTWBuild, Warning, TEXT("[건설] 설치 실패: 자원 부족"));
		return;
	}
	
	Server_SpawnBuilding(CurrentAnchor,CurrentGhost->BuildingSize, SelectedBuildingClass);	
	
	EndBuildMode();
	
}

void UTWBuildComponent::EndBuildMode()
{
	bIsBuildMode = false;
	
	if (CurrentGhost != nullptr)
	{
		CurrentGhost->Destroy();
		CurrentGhost = nullptr;
	}
	SelectedBuildingClass = nullptr;
}

void UTWBuildComponent::Server_SpawnBuilding_Implementation(FIntPoint Anchor, FIntPoint BuildSize,
	TSubclassOf<ATWBaseBuilding> ClassToSpawn)
{
	auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();
	if (!GridSub)
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}
	
	ATWPlayerState* TWPS = PC->GetPlayerState<ATWPlayerState>();
	if (!TWPS)
	{
		return;
	}
	
	if (!ClassToSpawn)
	{
		return;
	}
	
	ATWBaseBuilding* DefaultBuilding = ClassToSpawn->GetDefaultObject<ATWBaseBuilding>();
	if (!DefaultBuilding || !DefaultBuilding->BuildingData)
	{
		return;
	}

	if (DefaultBuilding->IsA(ATWUpgradeBuilding::StaticClass()))
	{
		if (HasOwnedUpgradeBuilding(GetWorld(), TWPS->PlayerSlot))
		{
			UE_LOG(LogTWBuild, Warning, TEXT("[건설] 설치 실패: 연구소는 1개만 건설할 수 있습니다."));
			return;
		}
	}
	
	if (!GridSub->CanBuildArea(Anchor, BuildSize))
	{
		UE_LOG(LogTWBuild, Warning, TEXT("[건설] 설치 실패: 배치 불가 위치"));
		return;
	}
	
	if (TWPS->CanAffordCost(DefaultBuilding->BuildingData->BuildCost) == 0)
	{
		UE_LOG(LogTWBuild, Warning, TEXT("[건설] 설치 실패: 자원 부족"));
		return;
	}
	
	FVector SpawnPos = GridSub->GetBuildingCenterPosition(Anchor, BuildSize);
	ATWBaseBuilding* NewBuilding = GetWorld()->SpawnActor<ATWBaseBuilding>(ClassToSpawn, SpawnPos, FRotator::ZeroRotator);
	
	if (!NewBuilding)
	{
		UE_LOG(LogTWBuild, Warning, TEXT("[건설] 설치 실패: SpawnActor 실패"));
		return;
	}
	
	TWPS->SpendCost(DefaultBuilding->BuildingData->BuildCost);
	
	NewBuilding->SetOwnerPlayerSlot(TWPS->PlayerSlot);
	
	if (ATWGameMode* TWGM = GetWorld()->GetAuthGameMode<ATWGameMode>())
	{
		TWGM->TryBindBuilding(NewBuilding);
	}
		
	GridSub->OccupyArea(Anchor, BuildSize, NewBuilding);
}

