#pragma once

#include "CoreMinimal.h"
#include "TWUIInputStateTypes.generated.h"

UENUM(BlueprintType)
enum class ETWCursorVisualType : uint8
{
	Default,
	Move,
	Attack,
	Build,
	Forbidden,
	EdgeScroll
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FUICommandInputStateViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ArmedCommandId = NAME_None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString HintText = TEXT("");

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETWCursorVisualType CursorVisualType = ETWCursorVisualType::Default;
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FUIDragSelectionStateViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bVisible = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D StartScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D EndScreenPosition = FVector2D::ZeroVector;
};