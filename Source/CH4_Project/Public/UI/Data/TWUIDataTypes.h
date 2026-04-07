#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "TWUIDataTypes.generated.h"

UENUM(BlueprintType)
enum class ESelectionViewType : uint8
{
	None,
	Unit,
	Building,
	Multi
};

UENUM(BlueprintType)
enum class ESelectionViewMode : uint8
{
	None,
	Single,
	Multi
};

UENUM(BlueprintType)
enum class ENotificationType : uint8
{
	Info,
	Warning,
	Error
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTopBarViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Wood = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Gas = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Population = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString GameTimeText = TEXT("00:00");
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FSelectionSummaryItemViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName EntityId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DisplayName = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Count = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString CountText = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FSelectionViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESelectionViewType SelectionType = ESelectionViewType::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESelectionViewMode ViewMode = ESelectionViewMode::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName SelectionId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DisplayName = TEXT("No Selection");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString TypeLabel = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CurrentHP = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxHP = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HPText = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CurrentEnergy = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxEnergy = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> StatusIconIds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UTexture2D> PortraitIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 TotalSelectedCount = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString CountLabel = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FSelectionSummaryItemViewModel> SummaryItems;
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FCommandSlotViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 SlotIndex = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName CommandId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DisplayName = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HotkeyLabel = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Description = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bEnabled = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPending = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float CooldownRatio = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 QueueCount = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DisableReason = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsContextCommand = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bReturnToPreviousContext = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName NextContext = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FCommandHotkeyBinding
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName CommandId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FKey BoundKey;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DisplayLabel = TEXT("");
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FUICommandMetaRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName CommandId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 SlotIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText HotkeyLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName NextContext = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bIsContextCommand = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bReturnToPreviousContext = false;
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FUISelectionPresentationRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName EntityId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText TypeLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> PortraitIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UTexture2D> Portrait;
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FMenuButtonViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 SlotIndex = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ActionId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString DisplayName = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HotkeyLabel = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bEnabled = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString TooltipText = TEXT("");
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FMinimapPanelViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Title = TEXT("MINIMAP");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlaceholderText = TEXT("Minimap Placeholder");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bVisible = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bInteractive = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShowFrame = true;
};