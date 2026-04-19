#include "HeroUnit/TWHeroProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ATWHeroProjectile::ATWHeroProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>("CollisionComponent");
	CollisionComponent->InitSphereRadius(10.0f);
	
	CollisionComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);
	
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ATWHeroProjectile::OnOverlap);
	RootComponent = CollisionComponent;
	
	ProjectileMesh= CreateDefaultSubobject<UStaticMeshComponent>("ProjectileMesh");
	ProjectileMesh->SetupAttachment(RootComponent);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = this->InitSpeed;
	ProjectileMovement->MaxSpeed = this->MaxSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	
	InitialLifeSpan = 0.0f;
}

void ATWHeroProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void ATWHeroProjectile::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor || OtherActor != this)
	{
		return;
	}
	
	if (OverlappedActor.Contains(OtherActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("이미 관통한 액터"));
		return;
	}
	
	if (OtherActor->ActorHasTag(FName("Environment")))
	{
		UE_LOG(LogTemp, Warning, TEXT("지형에 부딛혀 Projectile제거"));
		Destroy();
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("액터 최초 관통"));
	OverlappedActor.Add(OtherActor);
}
