#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TWTeamComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamChanged, int32, NewTeamID);
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CH4_PROJECT_API UTWTeamComponent : public UActorComponent
{
	GENERATED_BODY()

public:    
	UTWTeamComponent();
	
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	UFUNCTION()
	void OnRep_TeamID();

	int32 GetTeamID();
	void SetTeamID(int32 NewTeamID);

	UPROPERTY(BlueprintAssignable, Category = "Team")
	FOnTeamChanged OnTeamChanged;
	
protected:
	// Replicated를 통해 동기화
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = -1;
	
	uint8 bIsTeamInitialized : 1;

};
