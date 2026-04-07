#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWCommandRequest.generated.h"

UENUM(BlueprintType)
enum class ERTSCommandTargetType : uint8
{
	None,
	WorldLocation,
	Actor
};

USTRUCT(BlueprintType)
struct CH4_PROJECT_API FTWCommandRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName CommandId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IssuedByPlayerId = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> SelectedEntityIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERTSCommandTargetType TargetType = ERTSCommandTargetType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector_NetQuantize TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OptionalContext = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ClientTimestamp = 0.0f;

	bool IsValidRequest() const
	{
		return !CommandId.IsNone();
	}
};
