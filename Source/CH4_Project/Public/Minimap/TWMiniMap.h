#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWMiniMap.generated.h"

UCLASS()
class CH4_PROJECT_API ATWMiniMap : public AActor
{
	GENERATED_BODY()
	
public:	
	ATWMiniMap();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	USceneCaptureComponent2D* CaptureComp;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	UTextureRenderTarget2D* MinimapRT;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Minimap")
	int32 CaptureWidth = 1024;
	
public:
	void UpdateCapture();
	
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	FVector GetWorldLocationFromTouch(FVector2D TouchPos, FVector2D WidgetSize);
	
private:
	FTimerHandle CaptureTimerHandle;
	
    FVector GridOrigin;
    FVector2D GridFullSize;
	
	int32 CachedLocalTeamSlot = -1; // 추가
	
#pragma region Outline
public:
	void DrawCameraFrustum();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minimap")
	UTextureRenderTarget2D* FrustumRT; // 외곽선 전용 렌더 타겟

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor FrustumColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float FrustumThickness = 2.0f;
#pragma endregion
	
#pragma region Icon
	void DrawIconsOnMinimap();
	FVector2D WorldToMinimap(FVector WorldPos, FVector2D CanvasSize) const;
	bool ShouldShow(int32 TeamID, FVector WorldPos, int32 LocalTeamSlot, class ATWFogManager* FogManager) const;

	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	UTextureRenderTarget2D* IconRT; // 유닛/건물 전용 렌더 타겟

	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	float UnitIconSize = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	float BuildingIconSize = 20.0f;
	
	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	float CapturePointIconSize = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	FLinearColor MyTeamColor = FLinearColor::Green;

	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	FLinearColor EnemyTeamColor = FLinearColor::Red;
	
	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	FLinearColor NeutralColor = FLinearColor::Yellow; // 중립 점령지 색상
	
	UPROPERTY(EditAnywhere, Category = "Minimap|Icons")
	float IconThickness = 15.f;
#pragma endregion
	
#pragma region 머티리얼
	UPROPERTY(EditAnywhere, Category="MiniMap|Water")
	UMaterialInterface* MinimapWaterMaterial = nullptr;

	UPROPERTY()
	TArray<TWeakObjectPtr<UStaticMeshComponent>> WaterMeshComponents;

	TMap<TWeakObjectPtr<UStaticMeshComponent>, TArray<UMaterialInterface*>> OriginalWaterMaterials;
	
	void CacheWaterActors();
	void ApplyMinimapWaterMaterial();
	void RestoreWaterMaterials();
#pragma endregion
};
