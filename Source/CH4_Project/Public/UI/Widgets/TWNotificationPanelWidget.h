#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWNotificationPanelWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class CH4_PROJECT_API UTWNotificationPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Notification")
	void ShowNotification(const FString& Message, ENotificationType Type);

	UFUNCTION(BlueprintCallable, Category="Notification")
	void SetModeNotificationText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category="Notification")
	void SetModeNotificationVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Notification")
	void ShowModeNotification(bool bIsBuildMode);

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TextMessage = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UBorder> NotificationBorder = nullptr;
};