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
	

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category=Component)
	TObjectPtr<UMassAgentComponent> MassAgentComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Anchor")
	TObjectPtr<USceneComponent> SelectionAnchor = nullptr;
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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor")
	bool bAutoPlaceAnchorsFromMeshBounds = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor", meta=(EditCondition="bAutoPlaceAnchorsFromMeshBounds"))
	float AutoSelectionAnchorZOffset = 4.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor", meta=(EditCondition="bAutoPlaceAnchorsFromMeshBounds"))
	float AutoHPBarExtraHeight = 24.f;

protected:
	void AutoPlaceAnchors();
};