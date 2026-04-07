#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWResourceDataProvider.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnUIResourceChanged);

UINTERFACE(BlueprintType)
class CH4_PROJECT_API UTWResourceDataProvider : public UInterface
{
	GENERATED_BODY()
};

class CH4_PROJECT_API ITWResourceDataProvider
{
	GENERATED_BODY()

public:
	// =========================================================
	// 현재 UI가 직접 사용하는 핵심 계약
	// =========================================================

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI Provider")
	FTopBarViewModel GetTopBarViewModel() const;

	// =========================================================
	// 실시간 표시 / 내부 갱신용 계약
	// =========================================================

	virtual int32 GetElapsedSeconds() const = 0;

	// =========================================================
	// 변경 알림 계약
	// Coordinator가 Resource Provider 변경 이벤트에 바인딩하기 위해 필요
	// =========================================================

	virtual FOnUIResourceChanged& GetOnResourceChangedDelegate() = 0;
};