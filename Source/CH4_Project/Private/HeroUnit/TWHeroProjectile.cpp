#include "HeroUnit/TWHeroProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ATWHeroProjectile::ATWHeroProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(10.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetGenerateOverlapEvents(false);
	RootComponent = CollisionComponent;
	
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetCastShadow(false);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = InitSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	
	InitialLifeSpan = 2.0f;
}

void ATWHeroProjectile::BeginPlay()
{
	Super::BeginPlay();
}

void ATWHeroProjectile::OnOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherOverlappedComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (OverlappedActor.Contains(OtherActor))
	{
		return;
	}

	OverlappedActor.Add(OtherActor);

	if (OtherActor->ActorHasTag(FName("Environment")))
	{
		Destroy();
		return;
	}
	
	Destroy();
}

void ATWHeroProjectile::InitializeVisualProjectile(const FVector& InTargetLocation)
{
	TargetLocation = InTargetLocation;

	const FVector StartLocation = GetActorLocation();
	const FVector MoveDirection = TargetLocation - StartLocation;
	const float Distance = MoveDirection.Size();

	if (MoveDirection.IsNearlyZero())
	{
		SetLifeSpan(0.15f);
		return;
	}

	SetActorRotation(MoveDirection.Rotation());

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = MoveDirection.GetSafeNormal() * ProjectileMovement->InitialSpeed;
	}

	const float TravelSpeed =
		ProjectileMovement
		? FMath::Max(1.f, ProjectileMovement->InitialSpeed)
		: FMath::Max(1.f, InitSpeed);

	const float LifeTime = FMath::Clamp((Distance / TravelSpeed) + 0.1f, 0.15f, 5.0f);
	SetLifeSpan(LifeTime);
}
