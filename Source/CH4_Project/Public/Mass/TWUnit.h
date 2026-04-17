// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassAgentComponent.h"
#include "TWUnit.generated.h"

class UAnimMontage;

UCLASS()
class CH4_PROJECT_API ATWUnit : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATWUnit();
	void OnConstruction(const FTransform& Transform);
	void BeginPlay();

protected:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category=Component)
	TObjectPtr<USceneComponent> SceneComponent;
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category=Component)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;
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
	USkeletalMeshComponent* GetUnitMeshComponent() const { return SkeletalMeshComponent; }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor")
	bool bAutoPlaceAnchorsFromMeshBounds = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor", meta=(EditCondition="bAutoPlaceAnchorsFromMeshBounds"))
	float AutoSelectionAnchorZOffset = 4.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MassVisual|Anchor", meta=(EditCondition="bAutoPlaceAnchorsFromMeshBounds"))
	float AutoHPBarExtraHeight = 24.f;

protected:
	void AutoPlaceAnchors();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimMontage)
	TObjectPtr<UAnimMontage> AttackMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AnimMontage)
	TObjectPtr<UAnimMontage> DeadMontage;
};
