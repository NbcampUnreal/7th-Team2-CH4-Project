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
};
