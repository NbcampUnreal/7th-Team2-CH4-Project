#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWBaseBuilding.generated.h"

class UTWBuildingDataAsset;
class ATWPlayerState;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class CH4_PROJECT_API ATWBaseBuilding : public AActor
{
	GENERATED_BODY()

public:
	ATWBaseBuilding();

	virtual void Destroyed() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Building")
	TObjectPtr<USceneComponent> SceneRoot;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Building")
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Building")
	TObjectPtr<UTWBuildingDataAsset> BuildingData = nullptr;
	
	// 0 = Player1, 1 = Player2 ... [테스트]
	UPROPERTY(Replicated, EditInstanceOnly, BlueprintReadOnly, Category="Building")
	int32 OwnerPlayerSlot = 0;
	
	UFUNCTION(BlueprintCallable, Category="Building")
	int32 GetOwnerPlayerSlot() const { return OwnerPlayerSlot; }

protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building")
	TObjectPtr<ATWPlayerState> OwningPlayerState = nullptr;

public:
	void SetOwnerPlayerState(ATWPlayerState* InPlayerState);
	ATWPlayerState* GetOwnerPlayerState() const { return OwningPlayerState; }

protected:
	virtual void OnOwnerPlayerStateAssigned();
	virtual void ClearAllBuildingTimers();
};