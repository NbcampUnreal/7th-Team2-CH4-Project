#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TW_TeamComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamChanged, int32, NewTeamID);
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CH4_PROJECT_API UTW_TeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UTW_TeamComponent();
	
	virtual void BeginPlay() override;

	// Replicated를 통해 동기화
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = 0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	void OnRep_TeamID();

	void SetTeamID(int32 NewTeamID);

	UPROPERTY(BlueprintAssignable, Category = "Team")
	FOnTeamChanged OnTeamChanged;
};
