#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "UI/Widgets/TWCursorOverlayWidget.h"
#include "UI/Widgets/TWDragSelectionOverlayWidget.h"
#include "TWMinimapPanelWidget.generated.h"

class UBorder;
class UTextBlock;
class UWidget;

UCLASS()
class CH4_PROJECT_API UTWMinimapPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetMinimapData(const FMinimapPanelViewModel& InData);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MinimapFrame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MinimapSurface;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> DisabledOverlay;
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTWCursorOverlayWidget> CursorOverlayWidget = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTWDragSelectionOverlayWidget> DragSelectionOverlayWidget = nullptr;
};