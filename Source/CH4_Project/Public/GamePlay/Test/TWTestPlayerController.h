#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/Data/TWUIDataTypes.h"
#include "TWTestPlayerController.generated.h"

class UDataTable;
class UTWHUDRootWidget;
class UTWUIHUDCoordinator;
class UTWUISelectionProvider;
class UTWUIResourceProvider;
class ATWTestUnitActor;
class ATWTestBuildingActor;

UCLASS()
class CH4_PROJECT_API ATWTestPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATWTestPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

protected:
	void CreateAndShowHUD();
	void CreateCoordinator();
	void BindCoordinator();
	void CreateProviders();
	void ShutdownProviders();

	void HandleLeftClick();
	void HandleRightClick();

	void Input_CommandSlot1();
	void Input_CommandSlot2();
	void Input_CommandSlot3();
	void Input_CommandSlot4();
	void Input_CommandSlot5();
	void Input_CommandSlot6();
	void Input_CommandSlot7();
	void Input_CommandSlot8();
	void Input_CommandSlot9();
	void Input_CommandSlot0();

	void HandleCommandSlotPressed(int32 InHumanReadableHotkey);
	bool TryHandleCommandBySlot(int32 InHumanReadableHotkey);
	bool TryHandleCommandById(FName InCommandId);
	void HandleCommandButtonClicked(FName InCommandId);

	void SetSelectedActor(AActor* InActor);
	void ClearSelection();

	bool TryBuildSelectionFromActor(
		AActor* InActor,
		FSelectionViewModel& OutSelectionVM,
		TArray<FName>& OutCommandIds) const;

	bool TryIssuePendingMove(const FVector& InTargetLocation);
	bool ExecuteGameplayCommandById(FName InCommandId);

	ATWTestUnitActor* GetSelectedTestUnit() const;
	ATWTestBuildingActor* GetSelectedTestBuilding() const;

	void PushInfoNotification(const FString& InMessage) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSubclassOf<UTWHUDRootWidget> HUDRootWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Data")
	TObjectPtr<UDataTable> CommandMetaTable = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI|Data")
	TObjectPtr<UDataTable> SelectionPresentationTable = nullptr;

	UPROPERTY()
	TObjectPtr<UTWHUDRootWidget> HUDRootWidget = nullptr;

	UPROPERTY()
	TObjectPtr<UTWUIHUDCoordinator> HUDCoordinator = nullptr;

	UPROPERTY()
	TObjectPtr<UTWUISelectionProvider> SelectionProvider = nullptr;

	UPROPERTY()
	TObjectPtr<UTWUIResourceProvider> ResourceProvider = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> CurrentSelectedActor = nullptr;

	UPROPERTY()
	bool bPendingMoveCommand = false;
};