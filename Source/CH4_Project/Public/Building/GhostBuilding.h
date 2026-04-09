#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostBuilding.generated.h"

UCLASS()
class CH4_PROJECT_API AGhostBuilding : public AActor
{
	GENERATED_BODY()

public:

	AGhostBuilding();

	virtual void OnConstruction(const FTransform& Transform) override;
	
	void SetBuildingMesh(UStaticMesh* Mesh, FIntPoint BuildingSize);
	void UpdateBuildingVisual(bool bCanBuild);
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BuildingMID;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> BaseBuildingMaterial;
	
};
