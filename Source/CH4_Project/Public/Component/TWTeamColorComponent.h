#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TWTeamColorComponent.generated.h"

class UMeshComponent;
class UMaterialInterface;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CH4_PROJECT_API UTWTeamColorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTWTeamColorComponent();

protected:
	
	virtual void BeginPlay() override;
	
public:
	
	void SetUpTargetMesh(UMeshComponent* InMeshComponent);

	void ApplyTeamColor(int32 PlayerSlot);
	void RestoreOriginalMaterials();
	
protected:
	
	UPROPERTY()
	TObjectPtr<UMeshComponent> TargetMesh = nullptr;
	
	UPROPERTY(EditAnywhere, Category="TeamColor")
	TMap<int32, TObjectPtr<UMaterialInterface>> PlayerMaterialMap;
	
	UPROPERTY(EditAnywhere, Category="TeamColor")
	int32 TargetMaterialIndex = 0;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
	
	uint8 bHasCachedMaterials : 1;
	
	void CacheOriginalMaterial();
	
};
