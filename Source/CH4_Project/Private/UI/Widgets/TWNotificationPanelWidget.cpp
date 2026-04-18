#include "UI/Widgets/TWNotificationPanelWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UTWNotificationPanelWidget::ShowNotification(const FString& Message, ENotificationType Type)
{
	if (TextMessage)
	{
		TextMessage->SetText(FText::FromString(Message));
	}

	if (NotificationBorder)
	{
		FLinearColor Color = FLinearColor(0.1f, 0.1f, 0.1f, 0.8f);

		switch (Type)
		{
		case ENotificationType::Info:
			Color = FLinearColor(0.1f, 0.1f, 0.1f, 0.8f);
			break;

		case ENotificationType::Warning:
			Color = FLinearColor(0.4f, 0.25f, 0.0f, 0.85f);
			break;

		case ENotificationType::Error:
			Color = FLinearColor(0.4f, 0.0f, 0.0f, 0.85f);
			break;
		}

		NotificationBorder->SetBrushColor(Color);
		NotificationBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTWNotificationPanelWidget::SetModeNotificationText(const FText& InText)
{
	ShowNotification(InText.ToString(), ENotificationType::Info);
}

void UTWNotificationPanelWidget::SetModeNotificationVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (NotificationBorder)
	{
		NotificationBorder->SetVisibility(
			bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed
		);
	}
}

void UTWNotificationPanelWidget::ShowModeNotification(bool bIsBuildMode)
{
	ShowNotification(
		bIsBuildMode ? TEXT("건설 모드") : TEXT("일반 모드"),
		ENotificationType::Info
	);
}