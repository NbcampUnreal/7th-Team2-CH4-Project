#include "UI/Widgets/TWSelectionQueueItemWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Log/TWLogCategory.h"
#include "UI/Data/TWSelectionQueueItemObject.h"

void UTWSelectionQueueItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTWSelectionQueueItemObject* ItemObject = Cast<UTWSelectionQueueItemObject>(ListItemObject);
	if (!ItemObject)
	{
		UE_LOG(LogTWUI, Warning, TEXT("[QueueItemWidget] Invalid ListItemObject"));
		return;
	}

	SetQueueItemData(ItemObject->QueueData);
	ApplyCurveVisual(ItemObject->QueueIndex, ItemObject->TotalCount);
}

float UTWSelectionQueueItemWidget::CalculateCurveOffsetY(int32 QueueIndex, int32 TotalCount) const
{
	// 아이템이 1개거나 비정상 값이면 곡선 적용 안 함
	if (QueueIndex < 0 || TotalCount <= 1)
	{
		return 0.f;
	}

	// 가운데 슬롯이 가장 아래로 내려가도록 아치형 오프셋 계산
	// 예: 5개일 때 0, 4, 8, 4, 0 느낌
	const float CenterIndex = (static_cast<float>(TotalCount) - 1.f) * 0.5f;
	const float DistanceFromCenter = FMath::Abs(static_cast<float>(QueueIndex) - CenterIndex);

	// 강도값. 필요 시 3~6 사이에서 조절
	const float CurveStep = 4.f;

	return (CenterIndex - DistanceFromCenter) * CurveStep;
}

void UTWSelectionQueueItemWidget::ApplyCurveVisual(int32 QueueIndex, int32 TotalCount)
{
	const float CurveOffsetY = CalculateCurveOffsetY(QueueIndex, TotalCount);

	// 슬롯이 살짝 아래로 눌려 배치되는 느낌
	SetRenderTranslation(FVector2D(0.f, CurveOffsetY));

	// 너무 과한 왜곡 방지용. 기본 배율 유지
	SetRenderScale(FVector2D(1.f, 1.f));
}

void UTWSelectionQueueItemWidget::SetQueueItemData(const FProductionQueueItemViewModel& InData)
{
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