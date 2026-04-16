#include "UI/Widgets/TWTopBarWidget.h"
#include "Styling/SlateColor.h"
#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"

#include "UI/Widgets/TWTopBarWidget.h"

#include "Components/TextBlock.h"

FString UTWTopBarWidget::FormatUpkeepText(int32 InUpkeep) const
{
	if (InUpkeep <= 0)
	{
		return TEXT("");
	}

	return FString::Printf(TEXT("(-%d)"), InUpkeep);
}

FSlateColor UTWTopBarWidget::ResolveUpkeepColor(int32 InUpkeep) const
{
	if (InUpkeep > 0)
	{
		return FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f, 1.0f));
	}

	return FSlateColor(FLinearColor::Transparent);
}

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

	if (TextMithril)
	{
		TextMithril->SetText(FText::AsNumber(InData.Mithril));
	}
	
	if (TextPopulation)
	{
		const FString PopulationDisplayText = ResolvePopulationDisplayText(InData);

		if (!PopulationDisplayText.IsEmpty())
		{
			TextPopulation->SetText(FText::FromString(PopulationDisplayText));
		}
		else
		{
			TextPopulation->SetText(FText::AsNumber(InData.Population));
		}
	}

	if (TextGameTime)
	{
		TextGameTime->SetText(FText::FromString(InData.GameTimeText));
	}

	if (TextWoodUpkeep)
	{
		const FString UpkeepText = FormatUpkeepText(InData.WoodUpkeep);
		TextWoodUpkeep->SetText(FText::FromString(UpkeepText));
		TextWoodUpkeep->SetColorAndOpacity(ResolveUpkeepColor(InData.WoodUpkeep));
		TextWoodUpkeep->SetVisibility(
			UpkeepText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible
		);
	}

	if (TextOreUpkeep)
	{
		const FString UpkeepText = FormatUpkeepText(InData.GasUpkeep);
		TextOreUpkeep->SetText(FText::FromString(UpkeepText));
		TextOreUpkeep->SetColorAndOpacity(ResolveUpkeepColor(InData.GasUpkeep));
		TextOreUpkeep->SetVisibility(
			UpkeepText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible
		);
	}
}

FString UTWTopBarWidget::ResolvePopulationDisplayText(const FTopBarViewModel& InData) const
{
	UScriptStruct* StructType = FTopBarViewModel::StaticStruct();
	if (!StructType)
	{
		return FString::FromInt(InData.Population);
	}

	const TArray<FName> CandidatePropertyNames = {
		TEXT("PopulationText"),
		TEXT("PopulationLabel"),
		TEXT("PopulationDisplayText")
	};

	for (const FName& PropertyName : CandidatePropertyNames)
	{
		if (const FStrProperty* StrProperty = FindFProperty<FStrProperty>(StructType, PropertyName))
		{
			const FString Value = StrProperty->GetPropertyValue_InContainer(&InData);
			if (!Value.IsEmpty())
			{
				return Value;
			}
		}

		if (const FTextProperty* TextProperty = FindFProperty<FTextProperty>(StructType, PropertyName))
		{
			const FString Value = TextProperty->GetPropertyValue_InContainer(&InData).ToString();
			if (!Value.IsEmpty())
			{
				return Value;
			}
		}
	}

	// fallback: 기존 숫자 단일 표시
	return FString::FromInt(InData.Population);
}
