#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TWPlayerController.generated.h"

class UInputAction;
struct FInputActionValue;
class UInputMappingContext;
class ATWPopulationBuilding;
class ATWBlockingBuilding;

UCLASS()
class CH4_PROJECT_API ATWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> IMC_Default = nullptr;
	
#pragma region 병력 스폰 대기열
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_TestSpawnTroop;

	UFUNCTION()
	void HandleTestSpawnTroop(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestSpawnTroop();
#pragma endregion

#pragma region 인구 수 대기열	
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_TestIncreasePopulation;
	
	UFUNCTION()
	void HandleTestIncreasePopulation(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestIncreasePopulation();
#pragma endregion
	
#pragma region 방벽 데미지
	UPROPERTY(EditDefaultsOnly, Category="Input")
	UInputAction* IA_TestDamageBlockingBuilding;

	UFUNCTION()
	void HandleTestDamageBlockingBuilding(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void ServerTestDamageBlockingBuilding();
#pragma endregion
};