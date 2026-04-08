#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TWTestUnitActor.generated.h"

UCLASS()
class CH4_PROJECT_API ATWTestUnitActor : public ACharacter
{
	GENERATED_BODY()

public:
	ATWTestUnitActor();

	FName GetUIEntityId() const;
	FString GetUIDisplayName() const;
	FString GetUITypeLabel() const;
	float GetCurrentHP() const;
	float GetMaxHP() const;

	TArray<FName> GetAvailableCommandIds() const;

	void MoveToWorldLocation(const FVector& InLocation);
	void StopUnit();
	void HoldPosition();
	void AttackTarget(AActor* InTarget);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FName UnitId = TEXT("Engineer");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FString DisplayName = TEXT("Engineer");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FString TypeLabel = TEXT("Worker");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TArray<FName> AvailableCommandIds = {
		TEXT("Move"),
		TEXT("Attack"),
		TEXT("Hold")
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float CurrentHPValue = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float MaxHPValue = 60.f;
};