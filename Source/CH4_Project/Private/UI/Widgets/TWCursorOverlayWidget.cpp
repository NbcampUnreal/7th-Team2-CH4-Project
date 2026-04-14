#include "UI/Widgets/TWCursorOverlayWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UTWCursorOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetPosition(CachedScreenPosition);
	}
}

void UTWCursorOverlayWidget::SetInputState(const FUICommandInputStateViewModel& InData)
{
	CachedInputState = InData;

	// 커서는 기본 상태에서도 항상 보이게 유지
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (TextHint)
	{
		const bool bShowHint = !InData.HintText.IsEmpty();

		TextHint->SetText(FText::FromString(InData.HintText));
		TextHint->SetVisibility(
			bShowHint
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed
		);
	}

	ApplyCursorVisual(InData.CursorVisualType);
}

void UTWCursorOverlayWidget::SetCursorScreenPosition(const FVector2D& InScreenPosition)
{
	CachedScreenPosition = InScreenPosition;
}

void UTWCursorOverlayWidget::ApplyCursorVisual(ETWCursorVisualType InType)
{
	if (!ImageCursor)
	{
		return;
	}

	TSoftObjectPtr<UTexture2D> TextureToUse;

	switch (InType)
	{
	case ETWCursorVisualType::Move:
		TextureToUse = MoveCursorTexture;
		break;

	case ETWCursorVisualType::Attack:
		TextureToUse = AttackCursorTexture;
		break;

	case ETWCursorVisualType::Build:
		TextureToUse = BuildCursorTexture;
		break;

	case ETWCursorVisualType::Forbidden:
		TextureToUse = ForbiddenCursorTexture;
		break;

	case ETWCursorVisualType::EdgeScroll:
		TextureToUse = EdgeScrollCursorTexture;
		break;

	case ETWCursorVisualType::Default:
	default:
		TextureToUse = DefaultCursorTexture;
		break;
	}

	UTexture2D* LoadedTexture = nullptr;

	if (!TextureToUse.IsNull())
	{
		LoadedTexture = TextureToUse.LoadSynchronous();
	}

	ImageCursor->SetBrushFromTexture(LoadedTexture, true);
	ImageCursor->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTWCursorOverlayWidget::SetCursorVisible(bool bInVisible)
{
	SetVisibility(
		bInVisible
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed
	);
}