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
	UFUNCTION(BlueprintCallable)
	void ShowNotification(const FString& Message, ENotificationType Type);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> NotificationBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TextMessage;
};