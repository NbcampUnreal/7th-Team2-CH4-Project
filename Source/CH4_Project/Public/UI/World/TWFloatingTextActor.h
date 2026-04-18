#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWFloatingTextActor.generated.h"

class USceneComponent;
class UTextRenderComponent;

UCLASS()
class CH4_PROJECT_API ATWFloatingTextActor : public AActor
{
	GENERATED_BODY()

public:
	ATWFloatingTextActor();

	void InitializeText(
		const FString& InMessage,
		const FColor& InColor,
		float InLifetime = 0.9f,
		float InRiseSpeed = 35.0f
	);

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	void UpdateFloatingText();
	void FinishFloatingText();
	void UpdateFacingToLocalCamera();

protected:
	UPROPERTY(VisibleAnywhere, Category = "FloatingText")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "FloatingText")
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY()
	FTimerHandle UpdateTimerHandle;

	UPROPERTY()
	FTimerHandle DestroyTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float UpdateInterval = 0.03f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float DefaultWorldSize = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float MinDistanceScale = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float MaxDistanceScale = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float DistanceScaleDivisor = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float DefaultXScale = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float DefaultYScale = 1.2f;

	float Lifetime = 0.9f;
	float RiseSpeed = 35.0f;
	float ElapsedTime = 0.0f;
};