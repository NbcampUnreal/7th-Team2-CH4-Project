#include "UI/Widgets/TWSelectionQueueItemWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Data/TWSelectionQueueItemObject.h"

void UTWSelectionQueueItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTWSelectionQueueItemObject* ItemObject = Cast<UTWSelectionQueueItemObject>(ListItemObject);
	if (!ItemObject)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QueueItemWidget] Invalid ListItemObject"));
		return;
	}

	SetQueueItemData(ItemObject->QueueData);
}

void UTWSelectionQueueItemWidget::SetQueueItemData(const FProductionQueueItemViewModel& InData)
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[QueueItemWidget] PayloadId=%s / DisplayName=%s / IconNull=%d / Active=%d / StackCount=%d"),
		*InData.PayloadId.ToString(),
		*InData.DisplayName,
		InData.Icon.IsNull() ? 1 : 0,
		InData.bIsActive ? 1 : 0,
		InData.StackCount
	);

	if (QueueIconImage)
	{
		QueueIconImage->SetVisibility(ESlateVisibility::Visible);
		QueueIconImage->SetRenderOpacity(1.f);
		QueueIconImage->SetColorAndOpacity(FLinearColor::White);

		UTexture2D* LoadedTexture = nullptr;

		if (!InData.Icon.IsNull())
		{
			LoadedTexture = InData.Icon.LoadSynchronous();
		}

		if (LoadedTexture)
		{
			QueueIconImage->SetBrushFromTexture(LoadedTexture, true);
		}
		else
		{
			QueueIconImage->SetBrushFromTexture(nullptr, true);
		}
	}

	if (ActiveBorder)
	{
		ActiveBorder->SetVisibility(
			InData.bIsActive
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed
		);
	}

	if (StackCountText)
	{
		if (InData.StackCount > 1)
		{
			StackCountText->SetText(FText::AsNumber(InData.StackCount));
			StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			StackCountText->SetText(FText::GetEmpty());
			StackCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}