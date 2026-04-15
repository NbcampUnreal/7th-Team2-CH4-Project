#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWBaseBuilding.generated.h"

class UTWBuildingDataAsset;
class ATWPlayerState;
class USceneComponent;
class UStaticMeshComponent;
class UTWTeamComponent;

UENUM(BlueprintType)
enum class ETWBuildingState : uint8
{
	None,
	UnderConstruction,
	Completed
};

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
	virtual void ApplyDamageToBuilding(const float InDamageAmount);

	UFUNCTION(BlueprintCallable, Category="Building|HP")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintCallable, Category="Building|HP")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintCallable, Category="Building|Construction")
	float GetBuildingProgress() const;
	
	UFUNCTION(BlueprintCallable, Category="Building|Construction")
	float GetRemainingBuildTime() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Team")
	TObjectPtr<UTWTeamComponent> TeamComponent = nullptr;

	UFUNCTION(BlueprintCallable, Category="Team")
	int32 GetTeamID() const;
	
protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building")
	TObjectPtr<ATWPlayerState> OwningPlayerState = nullptr;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building|HP")
	float MaxHP = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building|HP")
	float CurrentHP = 0.0f;
	
	UPROPERTY(ReplicatedUsing=OnRep_BuildingState, VisibleAnywhere, BlueprintReadOnly, Category="Building|Construction")
	ETWBuildingState BuildingState = ETWBuildingState::None;
	
	UFUNCTION()
	void OnRep_BuildingState();
	
	UPROPERTY(EditAnywhere, Category="Building|Visual")
	TObjectPtr<UMaterialInterface> ConstructionMaterial;
	
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
	
	FTimerHandle ConstructionTimerHandle;
	
	UPROPERTY(EditDefaultsOnly, Category="Building|Construction")
	float ConstructionTickInterval = 0.1f;
	
	float CurrentBuildTime = 0.0f;
	float MaxBuildTime = 0.0f;
	
	void StartConstruction();
	void UpdateConstruction();
	void FinishConstruction();
	
public:
	void SetOwnerPlayerState(ATWPlayerState* InPlayerState);
	ATWPlayerState* GetOwnerPlayerState() const { return OwningPlayerState; }

protected:
	virtual void HandleDestroyedByDamage();
	virtual void OnOwnerPlayerStateAssigned();
	virtual void ClearAllBuildingTimers();
};