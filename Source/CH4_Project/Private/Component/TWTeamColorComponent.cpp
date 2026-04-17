#include "Component/TWTeamColorComponent.h"


UTWTeamColorComponent::UTWTeamColorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bHasCachedMaterials = false;
	
	SetIsReplicatedByDefault(false);
}

void UTWTeamColorComponent::BeginPlay()
{
	Super::BeginPlay();
	
	CacheOriginalMaterial();
}

void UTWTeamColorComponent::SetUpTargetMesh(UMeshComponent* InMeshComponent)
{
	TargetMesh = InMeshComponent;
	
	CacheOriginalMaterial();
}

void UTWTeamColorComponent::ApplyTeamColor(int32 PlayerSlot)
{
	CacheOriginalMaterial();
	
	if (PlayerMaterialMap.Contains(PlayerSlot))
	{
		UMaterialInterface* PlayerMat = PlayerMaterialMap[PlayerSlot];
		
		if (PlayerMat && OriginalMaterials.IsValidIndex(TargetMaterialIndex))
		{
			OriginalMaterials[TargetMaterialIndex] = PlayerMat;
			
			if (TargetMesh)
			{
				TargetMesh->SetMaterial(TargetMaterialIndex, PlayerMat);
			}
		}
	}
}

void UTWTeamColorComponent::RestoreOriginalMaterials()
{
	if (!TargetMesh)
	{
		return;
	}
	
	for (int32 i = 0; i < OriginalMaterials.Num(); ++i)
	{
		if (OriginalMaterials.IsValidIndex(i) && OriginalMaterials[i] != nullptr)
		{
			TargetMesh->SetMaterial(i, OriginalMaterials[i]);
		}
	}
}

void UTWTeamColorComponent::CacheOriginalMaterial()
{
	if (bHasCachedMaterials || !TargetMesh)
	{
		return;
	}
	
	const int32 NumMats = TargetMesh->GetNumMaterials();
	for (int32 i = 0; i < NumMats; ++i)
	{
		OriginalMaterials.Add(TargetMesh->GetMaterial(i));
	}
	
	bHasCachedMaterials = true;
}

