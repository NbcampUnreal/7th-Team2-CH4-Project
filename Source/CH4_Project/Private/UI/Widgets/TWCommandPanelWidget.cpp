#include "UI/Widgets/TWCommandPanelWidget.h"

#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Log/TWLogCategory.h"
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
		return;
	}

	constexpr int32 NumColumns = 3;
	constexpr int32 NumRows = 3;
	constexpr int32 TotalSlots = NumColumns * NumRows;

	TArray<FCommandSlotViewModel> FixedSlots;
	FixedSlots.SetNum(TotalSlots);

	// 1) 기본적으로 9칸 모두 빈 슬롯으로 초기화
	for (int32 Index = 0; Index < TotalSlots; ++Index)
	{
		FCommandSlotViewModel& EmptyVM = FixedSlots[Index];
		EmptyVM.SlotIndex = Index;
		EmptyVM.CommandId = NAME_None;
		EmptyVM.DisplayName = TEXT("");
		EmptyVM.HotkeyLabel = TEXT("");
		EmptyVM.Description = TEXT("");
		EmptyVM.bVisible = true;   // 3x3 틀 유지용
		EmptyVM.bEnabled = false;  // 빈 슬롯은 클릭 불가
		EmptyVM.bArmed = false;
	}

	// 2) 실제 데이터는 SlotIndex 기준으로 덮어쓰기
	for (int32 SourceIndex = 0; SourceIndex < CachedCommandViewModels.Num(); ++SourceIndex)
	{
		const FCommandSlotViewModel& VM = CachedCommandViewModels[SourceIndex];

		int32 TargetIndex = VM.SlotIndex;
		if (TargetIndex < 0 || TargetIndex >= TotalSlots)
		{
			TargetIndex = SourceIndex;
		}

		if (TargetIndex >= 0 && TargetIndex < TotalSlots)
		{
			FixedSlots[TargetIndex] = VM;
			FixedSlots[TargetIndex].SlotIndex = TargetIndex;
		}
	}

	// 3) 항상 9개 슬롯 생성
	for (int32 Index = 0; Index < TotalSlots; ++Index)
	{
		UTWCommandSlotWidget* SlotWidget = CreateWidget<UTWCommandSlotWidget>(this, CommandSlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetCommandData(FixedSlots[Index]);
		BindSlotWidget(SlotWidget);

		if (UUniformGridSlot* GridSlot = SlotContainer->AddChildToUniformGrid(SlotWidget, Index / NumColumns, Index % NumColumns))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
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