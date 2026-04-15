#include "Building/GhostBuilding.h"

#include "Misc/MapErrors.h"


AGhostBuilding::AGhostBuilding()
{
	PrimaryActorTick.bCanEverTick = false;
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = StaticMesh;
	
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGhostBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	if (BaseBuildingMaterial && !BuildingMID)
	{
		BuildingMID = UMaterialInstanceDynamic::Create(BaseBuildingMaterial, this);
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

void AGhostBuilding::SetBuildingMesh(UStaticMesh* Mesh, FIntPoint NewBuildingSize)
{
	if (StaticMesh && Mesh)
	{
		StaticMesh->SetStaticMesh(Mesh);
		
		BuildingSize = NewBuildingSize;
		
		if (BuildingMID)
		{
			int32 MaterialCount = StaticMesh->GetNumMaterials();
			for (int32 i = 0; i < MaterialCount; ++i)
			{
				StaticMesh->SetMaterial(i, BuildingMID);
			}
		}
			
	}
}

void AGhostBuilding::UpdateBuildingVisual(bool bCanBuild)
{
	if (BuildingMID)
	{
		FLinearColor TargetColor = bCanBuild ? FLinearColor::Green : FLinearColor::Red;
		
		BuildingMID->SetVectorParameterValue(FName("TintColor"), TargetColor);
	}
}

