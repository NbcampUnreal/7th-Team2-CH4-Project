#include "Building/GhostBuilding.h"
#include "Building/TWBaseBuilding.h"
#include "Data/TWBuildingDataAsset.h"
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

void AGhostBuilding::InitGhsotBuilding(TSubclassOf<ATWBaseBuilding> BuildingClass)
{
	if (!BuildingClass)
	{
		return;
	}
	
	ATWBaseBuilding* DefaultBuilding = BuildingClass->GetDefaultObject<ATWBaseBuilding>();
	
	if (DefaultBuilding && DefaultBuilding->MeshComponent)
	{
		UStaticMesh* OriginalMesh = DefaultBuilding->MeshComponent->GetStaticMesh();
		
		FIntPoint OriginalSize(1, 1);
		if (DefaultBuilding->BuildingData)
		{
			OriginalSize = DefaultBuilding->BuildingData->GridSize.BuildingSize;
		}
	
		SetBuildingMesh(OriginalMesh, OriginalSize);
		
		StaticMesh->SetRelativeRotation(DefaultBuilding->MeshComponent->GetRelativeRotation());
		StaticMesh->SetRelativeScale3D(DefaultBuilding->MeshComponent->GetRelativeScale3D());
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

