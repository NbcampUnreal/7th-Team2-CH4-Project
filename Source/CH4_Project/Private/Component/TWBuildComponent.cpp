#include "Component/TWBuildComponent.h"
#include "GameFramework/PlayerController.h"
#include "Building/TWBaseBuilding.h"
#include "Building/GhostBuilding.h"
#include "Data/TWBuildingDataAsset.h"
#include "Subsystems/TWGridSubSystem.h"
#include "Components/StaticMeshComponent.h"
#include "Core/TWPlayerState.h"
#include "Core/TWGameMode.h"

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
	if (bIsBuildMode && CurrentGhost)
	{
		Server_SpawnBuilding(CurrentAnchor,CurrentGhost->BuildingSize, SelectedBuildingClass);
		
		auto* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>();
		if (GridSub)
		{
			GridSub->OccupyArea(CurrentAnchor, CurrentGhost->BuildingSize, nullptr);
		}
		
		EndBuildMode();
	}
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
	
	if (GridSub && GridSub->CanBuildArea(Anchor, BuildSize))
	{
		FVector SpawnPos = GridSub->GetBuildingCenterPosition(Anchor, BuildSize);
		ATWBaseBuilding* NewBuilding = GetWorld()->SpawnActor<ATWBaseBuilding>(ClassToSpawn, SpawnPos, FRotator::ZeroRotator);
		
		if (NewBuilding)
		{
			NewBuilding->OwnerPlayerSlot = TWPS->PlayerSlot;
		
			if (ATWGameMode* TWGM = GetWorld()->GetAuthGameMode<ATWGameMode>())
			{
				TWGM->TryBindBuilding(NewBuilding);
			}
		
			GridSub->OccupyArea(Anchor, BuildSize, NewBuilding);
		}
	}
}

