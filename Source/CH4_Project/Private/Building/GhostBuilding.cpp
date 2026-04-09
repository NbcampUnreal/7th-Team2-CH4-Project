#include "Building\GhostBuilding.h"

#include "Misc/MapErrors.h"


AGhostBuilding::AGhostBuilding()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGhostBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (BaseBuildingMaterial && !BuildingMID)
	{
		BuildingMID = UMaterialInstanceDynamic::Create(BuildingMID, this);
	}
	
	if (BuildingMID)
	{
		int32 MaterialCount = StaticMesh->GetNumMaterials();
		for (int32 i = 0; i < MaterialCount; ++i)
		{
			StaticMesh->SetMaterial(i, BuildingMID);
		}
	}
}

void AGhostBuilding::SetBuildingMesh(UStaticMesh* Mesh, FIntPoint BuildingSize)
{
}

void AGhostBuilding::UpdateBuildingVisual(bool bCanBuild)
{
}

