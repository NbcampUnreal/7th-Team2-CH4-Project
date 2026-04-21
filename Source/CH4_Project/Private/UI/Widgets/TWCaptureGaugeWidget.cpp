#include "UI/Widgets/TWCaptureGaugeWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UTWCaptureGaugeWidget::UpdateGauge(float Current, float Max, int32 TeamID)
{
	if (Current > 0.0f && TeamID < Max)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	
	float Percent = FMath::Clamp(Current/Max, 0.0f, 1.0f);
	if (CaptureProgressBar)
	{
		CaptureProgressBar->SetPercent(Percent);
		
		FLinearColor TeamColor = FLinearColor::Gray;
		if (TeamID == 0)
		{
			TeamColor = FLinearColor::Red;
		}
		else if (TeamID == 1)
		{
			TeamColor = FLinearColor::Blue;
		}
		
		CaptureProgressBar->SetFillColorAndOpacity(TeamColor);
	}
	
	if (ProgressText)
	{
		FString Text = FString::Printf(TEXT("%.f%%"), Percent * 100.0f);
		ProgressText->SetText(FText::FromString(Text));
	}
}
