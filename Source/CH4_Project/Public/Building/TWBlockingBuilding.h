#pragma once

#include "CoreMinimal.h"
#include "TWBaseBuilding.h"
#include "TWBlockingBuilding.generated.h"

class UTWBlockingBuildingDataAsset;

UCLASS()
class CH4_PROJECT_API ATWBlockingBuilding : public ATWBaseBuilding
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintCallable, Category="Building|HP")
	void ApplyDamageToBuilding(const int32 InDamageAmount);

	UFUNCTION(BlueprintCallable, Category="Building|HP")
	int32 GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintCallable, Category="Building|HP")
	int32 GetMaxHP() const { return MaxHP; }

protected:
	const UTWBlockingBuildingDataAsset* GetBlockingBuildingData() const;
	void HandleDestroyedByDamage();

protected:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building|HP")
	int32 MaxHP = 0;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Building|HP")
	int32 CurrentHP = 0;
};