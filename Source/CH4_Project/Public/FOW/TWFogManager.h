#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWFogManager.generated.h"

class UTWVisionComponent;

UCLASS()
class CH4_PROJECT_API ATWFogManager : public AActor
{
    GENERATED_BODY()

public:
    ATWFogManager();
    
    virtual void BeginPlay() override;
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
    TArray<TWeakObjectPtr<UTWVisionComponent>> RegisteredVisionComponents;
    
    UPROPERTY()
    UMaterialInstanceDynamic* FogPostProcessMID;

public:
    void RegisterVision(UTWVisionComponent* Comp);
    void UnregisterVision(UTWVisionComponent* Comp);
    
    // 시야 체크를 위해 캐싱된 픽셀 데이터
    TArray<FColor> CachedFogPixels;
    int32 CachedRTSizeX = 0;
    int32 CachedRTSizeY = 0;
    
    // 월드 좌표를 받아 현재 시야 확보 여부를 반환
    UFUNCTION(BlueprintPure, Category = "Fog")
    bool IsLocationVisible(const FVector& WorldLocation);
    
private:
    void UpdateFog();
    
    FVector2D WorldToUV(FVector WorldPos);
    
#pragma region EnemyVisibility
private:
    void UpdateEnemyVisibility();
#pragma endregion
};
