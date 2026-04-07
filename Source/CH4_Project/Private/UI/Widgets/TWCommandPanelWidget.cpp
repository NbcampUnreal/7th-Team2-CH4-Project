#include "UI/Widgets/TWCommandPanelWidget.h"

#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "UI/Widgets/TWCommandSlotWidget.h"

void UTWCommandPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UTWCommandPanelWidget::SetCommandData(const TArray<FCommandSlotViewModel>& InCommandViewModels)
{
	CachedCommandViewModels = InCommandViewModels;
	RebuildSlots();
}

void UTWCommandPanelWidget::RebuildSlots()
{
	if (!SlotContainer)
	{
		return;
	}

	SlotContainer->ClearChildren();
	SpawnedSlotWidgets.Reset();

	if (!CommandSlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RTSCommandPanelWidget] CommandSlotWidgetClass is null."));
		return;
	}

	constexpr int32 NumColumns = 3;

	for (int32 Index = 0; Index < CachedCommandViewModels.Num(); ++Index)
	{
		const FCommandSlotViewModel& VM = CachedCommandViewModels[Index];

		UTWCommandSlotWidget* SlotWidget = CreateWidget<UTWCommandSlotWidget>(this, CommandSlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetCommandData(VM);
		BindSlotWidget(SlotWidget);

		if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(SlotContainer->AddChild(SlotWidget)))
		{
			const int32 Row = Index / NumColumns;
			const int32 Column = Index % NumColumns;

			GridSlot->SetRow(Row);
			GridSlot->SetColumn(Column);
		}

		SpawnedSlotWidgets.Add(SlotWidget);
	}
}

void UTWCommandPanelWidget::BindSlotWidget(UTWCommandSlotWidget* InSlotWidget)
{
	if (!InSlotWidget)
	{
		return;
	}

	InSlotWidget->GetOnCommandSlotClickedDelegate().RemoveAll(this);
	InSlotWidget->GetOnCommandSlotClickedDelegate().AddUObject(
		this,
		&UTWCommandPanelWidget::HandleSlotCommandClicked
	);
}

void UTWCommandPanelWidget::HandleSlotCommandClicked(FName InCommandId)
{
	if (InCommandId.IsNone())
	{
		return;
	}

	OnCommandClicked.Broadcast(InCommandId);
}