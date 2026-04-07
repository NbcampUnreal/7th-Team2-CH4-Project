#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWTestBuildingActor.generated.h"

class USceneComponent;
class ATWTestUnitActor;

UCLASS()
class CH4_PROJECT_API ATWTestBuildingActor : public AActor
{
	GENERATED_BODY()

public:
	ATWTestBuildingActor();

protected:
	virtual void BeginPlay() override;

public:
	FName GetUIEntityId() const;
	FString GetUIDisplayName() const;
	FString GetUITypeLabel() const;
	float GetCurrentHP() const;
	float GetMaxHP() const;

	TArray<FName> GetAvailableCommandIds() const;
	int32 GetQueueCountForCommand(const FName& InCommandId) const;

	bool CanTrainFootman() const;
	bool StartTrainFootman();

	bool CanTrainArcher() const;
	bool StartTrainArcher();

protected:
	void FinishTrainFootman();
	void FinishTrainArcher();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* SpawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FName BuildingId = TEXT("Barracks");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FString DisplayName = TEXT("Barracks");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FString TypeLabel = TEXT("Military Building");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TArray<FName> AvailableCommandIds = {
		TEXT("TrainFootman"),
		TEXT("TrainArcher")
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float CurrentHPValue = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float MaxHPValue = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Train")
	TSubclassOf<ATWTestUnitActor> FootmanClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Train")
	TSubclassOf<ATWTestUnitActor> ArcherClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Train")
	float TrainDuration = 3.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Train")
	int32 FootmanQueueCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Train")
	int32 ArcherQueueCount = 0;

	bool bIsTraining = false;
	FTimerHandle TrainHandle;
};