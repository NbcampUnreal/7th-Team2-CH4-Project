// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TW_CapturePoint.generated.h"

class UWidgetComponent;
class UTWTeamColorComponent;
class UTWResourceBuildingDataAsset;
class UBoxComponent;
class UTWTeamComponent;
class UTWVisionComponent;
class ATWPlayerState;

UENUM()
enum class ECapturePointTier
{
	Small UMETA(DisplayName = "Small Point"),
	Big	UMETA(DisplayName = "Big Point"),
};

UCLASS()
class CH4_PROJECT_API ATW_CapturePoint : public AActor
{
	GENERATED_BODY()

public:
	ATW_CapturePoint();

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	
	UPROPERTY(VisibleAnywhere, Category = "Capture")
	UBoxComponent* CaptureArea;

	UPROPERTY(VisibleAnywhere, Category = "Capture")
	UTWTeamComponent* MyTeamComp;
	
	UPROPERTY(VisibleAnywhere, Category = "Capture")
	UTWVisionComponent* MyVisionComp;

	UPROPERTY(VisibleAnywhere, Category = "Capture")
	UStaticMeshComponent* MeshComp;
	
	UPROPERTY(VisibleAnywhere, Category = "Component|TeamColor")
	TObjectPtr<UTWTeamColorComponent> TeamColorComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture|Settings")
	ECapturePointTier PointTier = ECapturePointTier::Small;
	
	UPROPERTY(EditAnywhere, Category = "Capture")
	float MaxGauge = 100.0f;
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentGauge, BlueprintReadOnly, Category = "Capture")
	float CurrentGauge = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Capture")
	float CaptureSpeed = 10.0f; // 점령게이지 초당 상승량

	UPROPERTY(EditAnywhere, Category = "Capture")
	float DecaySpeed = 30.0f;    // 점령 게이지 감소량
	
	UPROPERTY(EditAnywhere, Category = "Capture")
	float VisionRad = 2000.0f;

private:
	FTimerHandle CaptureTimerHandle;
	const float CheckInterval = 0.1f;
	
	void StartCaptureTimer();
	
	void StopCaptureTimer();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void CheckCaptureStatus();
	
private:
	UPROPERTY(ReplicatedUsing=OnRep_CapturingTeamID, VisibleAnywhere, Category = "Capture")
	int32 CapturingTeamID = -1;

	// 영역 내 액터 관리
	UPROPERTY()
	TSet<AActor*> OverlappingActors;

	// 델리게이트 함수
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
#pragma region Resources
protected:
	FTimerHandle MithrilProductionTimerHandle;
	
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="Resource")
	TObjectPtr<ATWPlayerState> OwningPlayerState = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Resource")
	TObjectPtr<UTWResourceBuildingDataAsset> ResourceDataAsset;
	
	void SetOwningPlayer(ATWPlayerState* NewPlayerState);
	void StartMithrilProduction(); // 자원 생산 시작
	void HandleMithrilResource(); // 자원 지급
	
#pragma endregion
	
#pragma region Team Color
	
public:
	UPROPERTY(ReplicatedUsing=OnRep_OwnerPlayerSlot, EditInstanceOnly, BlueprintReadOnly, Category="Building")
	int32 OwnerPlayerSlot = -1;
	
	void SetOwnerPlayerSlot(int32 InSlot);
	
	UFUNCTION()
	void OnRep_OwnerPlayerSlot();
#pragma endregion
	
#pragma region UI
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> GaugeWidgetComp;
	
	UFUNCTION()
	void OnRep_CurrentGauge();

private:
	UFUNCTION()
	void OnRep_CapturingTeamID();
	
	void UpdateWidgetUI();
	void NotifyEnemyTeam(int32 InCapturingTeamID);
	
	uint8 bHasNotifiedCaptureStart : 1;
	
#pragma endregion
	
};
