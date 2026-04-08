#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Test_MyPlayerState.generated.h"

UCLASS()
class CH4_PROJECT_API ATest_MyPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ATest_MyPlayerState();
	
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadOnly, Category="Team")
	int32 TeamID = 0;
	
	UFUNCTION()
	void OnRep_TeamID();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
