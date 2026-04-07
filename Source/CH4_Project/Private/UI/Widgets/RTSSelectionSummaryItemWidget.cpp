#include "UI/Widgets/TWSelectionSummaryItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UTWSelectionSummaryItemWidget::SetSummaryData(const FSelectionSummaryItemViewModel& InData)
{
	if (TextName)
	{
		TextName->SetText(FText::FromString(InData.DisplayName));
	}

	if (TextCount)
	{
		const FString CountLabel =
			!InData.CountText.IsEmpty()
				? InData.CountText
				: FString::Printf(TEXT("x%d"), InData.Count);

		TextCount->SetText(FText::FromString(CountLabel));
	}

	if (IconImage)
	{
		UTexture2D* LoadedTexture = nullptr;

		if (!InData.Icon.IsNull())
		{
			LoadedTexture = InData.Icon.LoadSynchronous();
		}

		if (LoadedTexture)
		{
			IconImage->SetBrushFromTexture(LoadedTexture, true);
			IconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			IconImage->SetBrushFromTexture(nullptr, true);
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}