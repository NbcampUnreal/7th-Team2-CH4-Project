#include "UI/Widgets/TWTopBarWidget.h"

#include "Components/TextBlock.h"

void UTWTopBarWidget::SetTopBarData(const FTopBarViewModel& InData)
{
	if (TextWood)
	{
		TextWood->SetText(FText::AsNumber(InData.Wood));
	}

	if (TextOre)
	{
		TextOre->SetText(FText::AsNumber(InData.Gas));
	}

	if (TextPopulation)
	{
		TextPopulation->SetText(FText::AsNumber(InData.Population));
	}

	if (TextGameTime)
	{
		TextGameTime->SetText(FText::FromString(InData.GameTimeText));
	}
}