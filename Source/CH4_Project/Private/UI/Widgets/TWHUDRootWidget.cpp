#include "UI/Widgets/TWHUDRootWidget.h"

#include "UI/Widgets/TWCommandPanelWidget.h"
#include "UI/Widgets/TWMenuPanelWidget.h"
#include "UI/Widgets/TWMinimapPanelWidget.h"
#include "UI/Widgets/TWNotificationPanelWidget.h"
#include "UI/Widgets/TWSelectionPanelWidget.h"
#include "UI/Widgets/TWTopBarWidget.h"

void UTWHUDRootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CommandPanel)
	{
		CommandPanel->GetOnCommandClickedDelegate().RemoveAll(this);
		CommandPanel->GetOnCommandClickedDelegate().AddUObject(
			this,
			&UTWHUDRootWidget::HandleCommandPanelCommandClicked
		);
	}
}

void UTWHUDRootWidget::SetTopBarData(const FTopBarViewModel& InData)
{
	if (TopBar)
	{
		TopBar->SetTopBarData(InData);
	}
}

void UTWHUDRootWidget::SetSelectionData(const FSelectionViewModel& InData)
{
	if (SelectionPanel)
	{
		SelectionPanel->SetSelectionData(InData);
	}
}

void UTWHUDRootWidget::SetCommandData(const TArray<FCommandSlotViewModel>& InCommands)
{
	if (CommandPanel)
	{
		CommandPanel->SetCommandData(InCommands);
	}
}

void UTWHUDRootWidget::ShowNotification(const FString& Message, ENotificationType Type)
{
	if (NotificationPanel)
	{
		NotificationPanel->ShowNotification(Message, Type);
	}
}

void UTWHUDRootWidget::SetMenuData(const TArray<FMenuButtonViewModel>& InButtons)
{
	if (MenuPanel)
	{
		MenuPanel->SetMenuData(InButtons);
	}
}

void UTWHUDRootWidget::SetMinimapData(const FMinimapPanelViewModel& InData)
{
	if (MinimapPanel)
	{
		MinimapPanel->SetMinimapData(InData);
	}
}

void UTWHUDRootWidget::HandleCommandPanelCommandClicked(FName InCommandId)
{
	if (InCommandId.IsNone())
	{
		return;
	}

	OnHUDCommandClicked.Broadcast(InCommandId);
}