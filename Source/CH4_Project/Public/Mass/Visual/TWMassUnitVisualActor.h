#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassAgentComponent.h"
#include "TWMassUnitVisualActor.generated.h"

class USceneComponent;
class USkeletalMeshComponent;

UCLASS()
class CH4_PROJECT_API ATWMassUnitVisualActor : public AActor
{
	GENERATED_BODY()

public:
	ATWMassUnitVisualActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

public:
	/** 루트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	/** 실제 유닛 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent = nullptr;

	/**
	 * 선택 링 위치용 Anchor
	 * 보통 발 밑 중앙에 둔다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Anchor")
	TObjectPtr<USceneComponent> SelectionAnchor = nullptr;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category=Component)
	TObjectPtr<UMassAgentComponent> MassAgentComponent;

	/**
	 * HP Bar / Nameplate / 상태 아이콘 위치용 Anchor
	 * 보통 머리 위에 둔다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Anchor")
	TObjectPtr<USceneComponent> HPBarAnchor = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category="MassVisual|Anchor")
	USceneComponent* GetSelectionAnchorComponent() const { return SelectionAnchor; }

	UFUNCTION(BlueprintCallable, Category="MassVisual|Anchor")
	USceneComponent* GetHPBarAnchorComponent() const { return HPBarAnchor; }

	UFUNCTION(BlueprintCallable, Category="MassVisual|Anchor")
	FVector GetSelectionAnchorWorldLocation() const;

	UFUNCTION(BlueprintCallable, Category="MassVisual|Anchor")
	FVector GetHPBarAnchorWorldLocation() const;

	UFUNCTION(BlueprintCallable, Category="MassVisual|Anchor")
	FTransform GetSelectionAnchorWorldTransform() const;

	UFUNCTION(BlueprintCallable, Category="MassVisual|Anchor")
	FTransform GetHPBarAnchorWorldTransform() const;

	UFUNCTION(BlueprintCallable, Category="MassVisual|Mesh")
	USkeletalMeshComponent* GetUnitMeshComponent() const { return MeshComponent; }

	/**
	 * BP에서 Mesh / Anchor 배치를 아직 안 한 상태여도
	 * 기본값으로 대충 볼 수 있게 자동 세팅.
	 * 실제 프로젝트에서는 BP에서 수동 미세조정하는 걸 권장.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor")
	bool bAutoPlaceAnchorsFromMeshBounds = true;

	/**
	 * SelectionAnchor 기본 Z 보정
	 * 바닥에 너무 박히지 않게 약간 띄우는 용도
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor", meta=(EditCondition="bAutoPlaceAnchorsFromMeshBounds"))
	float AutoSelectionAnchorZOffset = 4.f;

	/**
	 * HPBarAnchor 기본 추가 높이
	 * 메시 상단보다 약간 더 위로
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor", meta=(EditCondition="bAutoPlaceAnchorsFromMeshBounds"))
	float AutoHPBarExtraHeight = 24.f;

protected:
	void AutoPlaceAnchors();
};