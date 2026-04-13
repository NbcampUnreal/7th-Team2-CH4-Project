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
	
private:
	FTimerHandle CaptureTimerHandle;
};
