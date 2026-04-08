#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWSelectionDataProvider.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnUISelectionChanged);
DECLARE_MULTICAST_DELEGATE(FOnUICommandContextChanged);

UINTERFACE(BlueprintType)
class CH4_PROJECT_API UTWSelectionDataProvider : public UInterface
{
	GENERATED_BODY()
};

class CH4_PROJECT_API ITWSelectionDataProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI Provider")
	FSelectionViewModel GetSelectionViewModel() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI Provider")
	TArray<FName> GetCurrentCommandIds() const;

	virtual ESelectionViewMode GetSelectionViewMode() const
	{
		return ESelectionViewMode::Single;
	}

	virtual FName GetPrimaryEntityId() const
	{
		return NAME_None;
	}

	virtual int32 GetTotalSelectedCount() const
	{
		return 0;
	}

	virtual TArray<FSelectionSummaryItemViewModel> GetSelectionSummary() const
	{
		return {};
	}

	virtual TArray<FName> GetCommandIdsForSelectionEntity(const FName& InEntityId) const
	{
		return {};
	}

	// =========================================================
	// 6단계 추가: Command Context
	// =========================================================

	virtual FName GetCurrentCommandContext() const
	{
		return NAME_None;
	}

	virtual TArray<FName> GetCommandIdsForCurrentContext() const
	{
		if (const UObject* AsObject = Cast<UObject>(const_cast<ITWSelectionDataProvider*>(this)))
		{
			return Execute_GetCurrentCommandIds(const_cast<UObject*>(AsObject));
		}
		return {};
	}

	virtual void HandleCommandInput(const FName& InCommandId, const FUICommandMetaRow* InCommandMetaRow = nullptr)
	{
	}

	virtual FOnUICommandContextChanged& GetOnCommandContextChangedDelegate()
	{
		static FOnUICommandContextChanged DummyDelegate;
		return DummyDelegate;
	}

	virtual FOnUISelectionChanged& GetOnSelectionChangedDelegate() = 0;
};