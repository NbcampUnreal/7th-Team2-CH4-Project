#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIInputStateTypes.h"
#include "TWCursorOverlayWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class CH4_PROJECT_API UTWCursorOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetInputState(const FUICommandInputStateViewModel& InData);
	void SetCursorScreenPosition(const FVector2D& InScreenPosition);
	UFUNCTION(BlueprintCallable)
	void SetCursorVisible(bool bInVisible);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ImageCursor = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextHint = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TSoftObjectPtr<UTexture2D> DefaultCursorTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TSoftObjectPtr<UTexture2D> MoveCursorTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TSoftObjectPtr<UTexture2D> AttackCursorTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TSoftObjectPtr<UTexture2D> BuildCursorTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TSoftObjectPtr<UTexture2D> ForbiddenCursorTexture;
	
	UPROPERTY(EditDefaultsOnly, Category = "Cursor")
	TSoftObjectPtr<UTexture2D> EdgeScrollCursorTexture;


private:
	void ApplyCursorVisual(ETWCursorVisualType InType);

private:
	FUICommandInputStateViewModel CachedInputState;
	FVector2D CachedScreenPosition = FVector2D::ZeroVector;
};