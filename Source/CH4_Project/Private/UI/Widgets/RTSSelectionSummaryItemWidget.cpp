#include "UI/Widgets/TWSelectionSummaryItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Data/TWSelectionSummaryItemObject.h"

void UTWSelectionSummaryItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTWSelectionSummaryItemObject* ItemObject = Cast<UTWSelectionSummaryItemObject>(ListItemObject);
	if (!ItemObject)
	{
		if (TextName)
		{
			TextName->SetText(FText::GetEmpty());
		}

		if (TextCount)
		{
			TextCount->SetText(FText::GetEmpty());
		}

		if (IconImage)
		{
			IconImage->SetBrushFromTexture(nullptr, true);
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		}

		return;
	}

	SetSummaryData(ItemObject->SummaryData);
}

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