#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TWBuildComponent.generated.h"

enum class EBuildingCategory : uint8;
class AGhostBuilding;
class ATWBaseBuilding;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CH4_PROJECT_API UTWBuildComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTWBuildComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	UFUNCTION(BlueprintCallable, Category = "Build")
	void SelectBuildingToConstruct(EBuildingCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Build")
	void RequestBuild();

	void EndBuildMode();
	
	bool GetBuildMode() const { return bIsBuildMode; }
	
protected:
	UFUNCTION(Server,Reliable, Category = "Build")
	void Server_SpawnBuilding(FIntPoint Anchor, FIntPoint BuildSize, TSubclassOf<ATWBaseBuilding> ClassToSpawn);
	
	UPROPERTY(EditDefaultsOnly, Category="Build|Map")
	TMap<EBuildingCategory, TSubclassOf<ATWBaseBuilding>> BuildingMap;
	
private:

	uint8 bIsBuildMode : 1;
	FIntPoint CurrentAnchor;
	
	UPROPERTY()
	TObjectPtr<AGhostBuilding> CurrentGhost;
	
	UPROPERTY(EditAnywhere, Category = "Build|Classes")
	TSubclassOf<AGhostBuilding> BuildClass;
	UPROPERTY(EditAnywhere, Category = "Build|Classes")
	TSubclassOf<ATWBaseBuilding> SelectedBuildingClass;
	
};
