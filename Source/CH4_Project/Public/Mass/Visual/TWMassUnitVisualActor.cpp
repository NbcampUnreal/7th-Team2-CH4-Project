#include "Mass/Visual/TWMassUnitVisualActor.h"
#include "MassAgentComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"

ATWMassUnitVisualActor::ATWMassUnitVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(SceneRoot);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(false);
	MassAgentComponent = CreateDefaultSubobject<UMassAgentComponent>(TEXT("MassAgentComponent"));
	SelectionAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionAnchor"));
	SelectionAnchor->SetupAttachment(MeshComponent);

	HPBarAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("HPBarAnchor"));
	HPBarAnchor->SetupAttachment(MeshComponent);

	// 기본값
	SelectionAnchor->SetRelativeLocation(FVector(0.f, 0.f, 4.f));
	HPBarAnchor->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
}

void ATWMassUnitVisualActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bAutoPlaceAnchorsFromMeshBounds)
	{
		AutoPlaceAnchors();
	}
}

void ATWMassUnitVisualActor::BeginPlay()
{
	Super::BeginPlay();

	// 에디터에서 BP 배치만으로 끝내면 BeginPlay에서는 굳이 다시 안 건드려도 되지만
	// 런타임 생성 시 메시 세팅 이후 bounds가 반영되는 경우도 있어서 한 번 더 보정.
	if (bAutoPlaceAnchorsFromMeshBounds)
	{
		AutoPlaceAnchors();
	}
}

FVector ATWMassUnitVisualActor::GetSelectionAnchorWorldLocation() const
{
	if (IsValid(SelectionAnchor))
	{
		return SelectionAnchor->GetComponentLocation();
	}

	if (IsValid(MeshComponent))
	{
		const FBoxSphereBounds Bounds = MeshComponent->Bounds;
		return FVector(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z - Bounds.BoxExtent.Z + AutoSelectionAnchorZOffset);
	}

	return GetActorLocation();
}

FVector ATWMassUnitVisualActor::GetHPBarAnchorWorldLocation() const
{
	if (IsValid(HPBarAnchor))
	{
		return HPBarAnchor->GetComponentLocation();
	}

	if (IsValid(MeshComponent))
	{
		const FBoxSphereBounds Bounds = MeshComponent->Bounds;
		return FVector(Bounds.Origin.X, Bounds.Origin.Y, Bounds.Origin.Z + Bounds.BoxExtent.Z + AutoHPBarExtraHeight);
	}

	return GetActorLocation() + FVector(0.f, 0.f, 120.f);
}

FTransform ATWMassUnitVisualActor::GetSelectionAnchorWorldTransform() const
{
	if (IsValid(SelectionAnchor))
	{
		return SelectionAnchor->GetComponentTransform();
	}

	return FTransform(GetActorRotation(), GetSelectionAnchorWorldLocation(), FVector::OneVector);
}

FTransform ATWMassUnitVisualActor::GetHPBarAnchorWorldTransform() const
{
	if (IsValid(HPBarAnchor))
	{
		return HPBarAnchor->GetComponentTransform();
	}

	return FTransform(GetActorRotation(), GetHPBarAnchorWorldLocation(), FVector::OneVector);
}

void ATWMassUnitVisualActor::AutoPlaceAnchors()
{
	if (!IsValid(MeshComponent))
	{
		return;
	}

	if (!MeshComponent->GetSkeletalMeshAsset())
	{
		return;
	}

	const FBoxSphereBounds Bounds = MeshComponent->CalcBounds(MeshComponent->GetComponentTransform());

	// MeshComponent 기준 상대 위치로 넣어야 해서 월드 기준 bounds를 다시 로컬 기준으로 변환
	const FVector MeshWorldOrigin = Bounds.Origin;
	const FVector MeshWorldBottom = MeshWorldOrigin - FVector(0.f, 0.f, Bounds.BoxExtent.Z);
	const FVector MeshWorldTop = MeshWorldOrigin + FVector(0.f, 0.f, Bounds.BoxExtent.Z);

	const FVector LocalBottom = MeshComponent->GetComponentTransform().InverseTransformPosition(MeshWorldBottom);
	const FVector LocalTop = MeshComponent->GetComponentTransform().InverseTransformPosition(MeshWorldTop);

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