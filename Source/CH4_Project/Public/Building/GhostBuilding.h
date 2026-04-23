#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostBuilding.generated.h"

class ATWBaseBuilding;

UCLASS()
class CH4_PROJECT_API AGhostBuilding : public AActor
{
	GENERATED_BODY()

public:

	AGhostBuilding();

	virtual void OnConstruction(const FTransform& Transform) override;
	
	UFUNCTION(BlueprintCallable, Category="GhostBuilding")
	void InitGhsotBuilding(TSubclassOf<ATWBaseBuilding> BuildingClass);
	
	void SetBuildingMesh(UStaticMesh* Mesh, FIntPoint NewBuildingSize);
	void UpdateBuildingVisual(bool bCanBuild);
	
	UPROPERTY(EditAnywhere, Category = "Grid")
	FIntPoint BuildingSize;
	
private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> StaticMesh;
	
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BuildingMID;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> BaseBuildingMaterial;
	
};
