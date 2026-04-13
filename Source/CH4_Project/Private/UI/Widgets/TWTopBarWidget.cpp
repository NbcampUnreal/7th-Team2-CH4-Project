#include "UI/Widgets/TWTopBarWidget.h"

#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"

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
		if (!InData.PopulationText.IsEmpty())
		{
			TextPopulation->SetText(FText::FromString(InData.PopulationText));
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