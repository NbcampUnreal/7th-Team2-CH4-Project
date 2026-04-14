#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIInputStateTypes.h"
#include "TWDragSelectionOverlayWidget.generated.h"

class UBorder;

UCLASS()
class CH4_PROJECT_API UTWDragSelectionOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDragState(const FUIDragSelectionStateViewModel& InData);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Top = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Bottom = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Left = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Right = nullptr;

private:
	void UpdateRect();

private:
	FUIDragSelectionStateViewModel CachedDragState;
};