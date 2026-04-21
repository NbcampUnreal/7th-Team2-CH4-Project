#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TWHeroProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class CH4_PROJECT_API ATWHeroProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ATWHeroProjectile();
	
	UFUNCTION(BlueprintCallable)
	void InitializeVisualProjectile(const FVector& InTargetLocation);
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category="Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;
	
	UPROPERTY(VisibleAnywhere, Category="Projectile")
	UProjectileMovementComponent* ProjectileMovement;
	
	UPROPERTY(VisibleAnywhere, Category="Projectile")
	TArray<AActor*> OverlappedActor;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	UStaticMeshComponent* ProjectileMesh;
	
	UFUNCTION()
	void OnOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherOverlappedComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);



protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Move")
	float InitSpeed = 2000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Move")
	float MaxSpeed = 2000.f;

	UPROPERTY()
	FVector TargetLocation = FVector::ZeroVector;
};
