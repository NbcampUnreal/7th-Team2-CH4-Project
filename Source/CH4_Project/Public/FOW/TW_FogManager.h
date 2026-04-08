#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TW_FogManager.generated.h"

class UTW_VisionComponent;

UCLASS()
class CH4_PROJECT_API ATW_FogManager : public AActor
{
    GENERATED_BODY()

public:
    ATW_FogManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite, Category="Fog")
    UTextureRenderTarget2D* CurrentFogRT;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog")
    UTextureRenderTarget2D* ExploredFogRT;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog")
    UMaterialInterface* DrawMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fog")
    UMaterialInterface* CombineMaterial;

    UPROPERTY()
    UMaterialInstanceDynamic* DrawMID;

    UPROPERTY()
    UMaterialInstanceDynamic* CombineMID;
    
    UPROPERTY(EditAnywhere, Category="Fog")
    UMaterialInterface* FogPostProcessMaterial;
    
    UPROPERTY(EditAnywhere, Category = "Fog")
    UMaterialParameterCollection* FogMPC;

    UPROPERTY(EditAnywhere)
    FVector2D MapSize = FVector2D(25000.f, 25000.f);

    UPROPERTY(EditAnywhere)
    FVector2D MapOrigin = FVector2D(0.f, 0.f);

    UPROPERTY(EditAnywhere)
    float UpdateInterval = 0.1f;

private:
    float AccumulatedTime = 0.f;

    UPROPERTY()
    TArray<TWeakObjectPtr<UTW_VisionComponent>> RegisteredVisionComponents;
    
    UPROPERTY()
    UMaterialInstanceDynamic* FogPostProcessMID;

public:
    void RegisterVision(UTW_VisionComponent* Comp);
    void UnregisterVision(UTW_VisionComponent* Comp);

private:
    void UpdateFog();
    FVector2D WorldToUV(FVector WorldPos);
};
