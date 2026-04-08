#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Core/TWResourceDataProvider.h"
#include "TWUIResourceProvider.generated.h"

UCLASS(BlueprintType)
class CH4_PROJECT_API UTWUIResourceProvider
	: public UObject
	, public ITWResourceDataProvider
{
	GENERATED_BODY()

public:
	UTWUIResourceProvider();

	virtual UWorld* GetWorld() const override;

	void Initialize(UObject* InResourceSystemSource);
	void Shutdown();

	UFUNCTION(BlueprintCallable, Category = "RTS UI|Real")
	void SetUseDebugFallback(bool bInUseDebugFallback);

	UFUNCTION(BlueprintCallable, Category = "RTS UI|Real")
	bool IsUsingDebugFallback() const;

	UFUNCTION(BlueprintCallable, Category = "RTS UI|Real")
	void DebugSetResources(const FTopBarViewModel& InTopBarViewModel, int32 InElapsedSeconds);

	UFUNCTION(BlueprintCallable, Category = "RTS UI|Real")
	void SetAutoAdvanceTime(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category = "RTS UI|Real")
	void SetAutoAdvanceInterval(float InSeconds);

	// =========================================================
	// IUIResourceDataProvider
	// =========================================================
	virtual FTopBarViewModel GetTopBarViewModel_Implementation() const override;
	virtual int32 GetElapsedSeconds() const override;
	virtual FOnUIResourceChanged& GetOnResourceChangedDelegate() override;

protected:
	void ApplyAutoAdvanceState();
	void HandleAutoAdvanceTick();
	void RefreshFromRealSourceIfAvailable();

protected:
	UPROPERTY()
	TObjectPtr<UObject> ResourceSystemSource = nullptr;

	UPROPERTY()
	FTopBarViewModel DebugTopBarViewModel;

	UPROPERTY()
	int32 DebugElapsedSeconds = 0;

	UPROPERTY(EditAnywhere, Category = "RTS UI|Real")
	bool bUseDebugFallback = false;

	UPROPERTY(EditAnywhere, Category = "RTS UI|Real")
	bool bAutoAdvanceTime = true;

	UPROPERTY(EditAnywhere, Category = "RTS UI|Real", meta = (ClampMin = "0.05"))
	float AutoAdvanceInterval = 1.0f;

	FTimerHandle AutoAdvanceTimerHandle;
	FOnUIResourceChanged OnResourceChanged;
};