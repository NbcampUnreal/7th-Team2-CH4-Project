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

	virtual void BeginPlay() override;
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
	
	UFUNCTION(BlueprintCallable, Category="Building|HP")
	virtual void ApplyDamageToBuilding(const int32 InDamageAmount);

	UFUNCTION(BlueprintCallable, Category="Building|HP")
	int32 GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintCallable, Category="Building|HP")
	int32 GetMaxHP() const { return MaxHP; }

protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building")
	TObjectPtr<ATWPlayerState> OwningPlayerState = nullptr;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building|HP")
	int32 MaxHP = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building|HP")
	int32 CurrentHP = 0;
	
public:
	void SetOwnerPlayerState(ATWPlayerState* InPlayerState);
	ATWPlayerState* GetOwnerPlayerState() const { return OwningPlayerState; }

protected:
	virtual void HandleDestroyedByDamage();
	virtual void OnOwnerPlayerStateAssigned();
	virtual void ClearAllBuildingTimers();
};