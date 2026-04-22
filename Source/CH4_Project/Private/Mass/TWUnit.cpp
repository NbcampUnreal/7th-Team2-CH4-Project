// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/TWUnit.h"

#include "Animation/TWUnitAnimInstance.h"
#include "Component/TWTeamColorComponent.h"
#include "Component/TWTeamComponent.h"
#include "FOW/TWVisionComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ATWUnit::ATWUnit()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SceneComponent = CreateDefaultSubobject<USceneComponent>("SceneComponent");
	SetRootComponent(SceneComponent);
	
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
	SkeletalMeshComponent->SetupAttachment(GetRootComponent());
	
	
	MassAgentComponent = CreateDefaultSubobject<UMassAgentComponent>("MassAgentComponent");
	SelectionAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionAnchor"));
	SelectionAnchor->SetupAttachment(SkeletalMeshComponent);
	
	HPBarAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("HPBarAnchor"));
	HPBarAnchor->SetupAttachment(SkeletalMeshComponent);

	TeamComponent = CreateDefaultSubobject<UTWTeamComponent>(TEXT("TeamComponent"));
	
	TeamColorComponent = CreateDefaultSubobject<UTWTeamColorComponent>(TEXT("TeamColorComponent"));
	
	FogVisionComponent = CreateDefaultSubobject<UTWVisionComponent>(TEXT("FogVisionComponent"));
	FogVisionComponent->VisionRadius = 800.f;
	
	Tags.AddUnique(FName("Unit"));
	
	// 기본값
	SelectionAnchor->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
	HPBarAnchor->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
}

void ATWUnit::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bAutoPlaceAnchorsFromMeshBounds)
	{
		AutoPlaceAnchors();
	}
}

void ATWUnit::BeginPlay()
{
	Super::BeginPlay();
	
	if (bAutoPlaceAnchorsFromMeshBounds)
	{
		AutoPlaceAnchors();
	}

	if (TeamComponent)
	{
		TeamComponent->OnTeamChanged.AddDynamic(this, &ATWUnit::OnTeamChangedCallBack);
	}
}

void ATWUnit::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (TeamColorComponent && SkeletalMeshComponent)
	{
		TeamColorComponent->SetUpTargetMesh(SkeletalMeshComponent);
	}
}

void ATWUnit::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ATWUnit, OwnerPlayerSlot);
}

void ATWUnit::PlayAttackMontage()
{
	if (SkeletalMeshComponent)
	{
		if (UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Play(AttackMontage);
		}
	}
}

void ATWUnit::PlayDeathMontage()
{
	if (SkeletalMeshComponent)
	{
		if (UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_Play(DeadMontage);
			if (UTWUnitAnimInstance* UnitAnimInstance = Cast<UTWUnitAnimInstance>(AnimInstance))
			{
				UnitAnimInstance->SetIsDead();
			}
		}
	}
}

FVector ATWUnit::GetSelectionAnchorWorldLocation() const
{
	if (IsValid(SelectionAnchor))
	{
		return SelectionAnchor->GetComponentLocation();
	}

	if (IsValid(SkeletalMeshComponent))
	{
		const FBoxSphereBounds Bounds = SkeletalMeshComponent->Bounds;
		return FVector(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - Bounds.BoxExtent.Z + AutoSelectionAnchorZOffset);
	}

	return GetActorLocation();
}

FVector ATWUnit::GetHPBarAnchorWorldLocation() const
{
	if (IsValid(HPBarAnchor))
	{
		return HPBarAnchor->GetComponentLocation();
	}

	if (IsValid(SkeletalMeshComponent))
	{
		const FBoxSphereBounds Bounds = SkeletalMeshComponent->Bounds;
		return FVector(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + Bounds.BoxExtent.Z + AutoHPBarExtraHeight);
	}

	return GetActorLocation() + FVector(0.f, 0.f, 120.f);
}

FTransform ATWUnit::GetSelectionAnchorWorldTransform() const
{
	if (IsValid(SelectionAnchor))
	{
		return SelectionAnchor->GetComponentTransform();
	}

	return FTransform(GetActorRotation(), GetSelectionAnchorWorldLocation(), FVector::OneVector);
}

FTransform ATWUnit::GetHPBarAnchorWorldTransform() const
{
	if (IsValid(HPBarAnchor))
	{
		return HPBarAnchor->GetComponentTransform();
	}

	return FTransform(GetActorRotation(), GetHPBarAnchorWorldLocation(), FVector::OneVector);
}

void ATWUnit::OnRep_OwnerPlayerSlot()
{
	if (TeamColorComponent)
	{
		TeamColorComponent->ApplyTeamColor(OwnerPlayerSlot);
	}
}

void ATWUnit::OnTeamChangedCallBack(int32 NewTeamID)
{
	SetOwnerPlayerSlot(NewTeamID);
}

void ATWUnit::SetOwnerPlayerSlot(int32 InSlot)
{
	if (HasAuthority())
	{
		OwnerPlayerSlot = InSlot;
		
		if (TeamColorComponent)
		{
			TeamColorComponent->ApplyTeamColor(OwnerPlayerSlot);
		}
	}
}

void ATWUnit::AutoPlaceAnchors()
{
	if (!IsValid(SkeletalMeshComponent))
	{
		return;
	}

	if (!SkeletalMeshComponent->GetSkeletalMeshAsset())
	{
		return;
	}

	const FBoxSphereBounds Bounds = SkeletalMeshComponent->CalcBounds(SkeletalMeshComponent->GetComponentTransform());

	// MeshComponent 기준 상대 위치로 넣어야 해서 월드 기준 bounds를 다시 로컬 기준으로 변환
	const FVector MeshWorldOrigin = Bounds.Origin;
	const FVector MeshWorldBottom = MeshWorldOrigin - FVector(0.f, 0.f, Bounds.BoxExtent.Z);
	const FVector MeshWorldTop = MeshWorldOrigin + FVector(0.f, 0.f, Bounds.BoxExtent.Z);

	const FVector LocalBottom = SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(MeshWorldBottom);
	const FVector LocalTop = SkeletalMeshComponent->GetComponentTransform().InverseTransformPosition(MeshWorldTop);

	if (IsValid(SelectionAnchor))
	{
		SelectionAnchor->SetRelativeLocation(
			FVector(
				0.f,
				0.f,
				LocalBottom.Z + AutoSelectionAnchorZOffset
			)
		);
	}

	if (IsValid(HPBarAnchor))
	{
		HPBarAnchor->SetRelativeLocation(
			FVector(
				0.f,
				0.f,
				LocalTop.Z + AutoHPBarExtraHeight
			)
		);
	}
}

