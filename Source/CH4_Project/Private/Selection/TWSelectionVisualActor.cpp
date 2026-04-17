#include "Selection/TWSelectionVisualActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Core/TWPlayerController.h"
#include "Core/TWPlayerState.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr int32 HPBAR_CUSTOMDATA_HEALTH_PERCENT = 0;
	constexpr int32 HPBAR_CUSTOMDATA_COLOR_R        = 1;
	constexpr int32 HPBAR_CUSTOMDATA_COLOR_G        = 2;
	constexpr int32 HPBAR_CUSTOMDATA_COLOR_B        = 3;
	constexpr int32 HPBAR_CUSTOMDATA_IS_BUILDING    = 4;
	constexpr int32 HPBAR_NUM_CUSTOMDATA_FLOATS     = 5;
}

ATWSelectionVisualActor::ATWSelectionVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	UnitRingISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("UnitRingISM"));
	UnitRingISM->SetupAttachment(SceneRoot);
	UnitRingISM->SetMobility(EComponentMobility::Movable);
	UnitRingISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UnitRingISM->SetGenerateOverlapEvents(false);
	UnitRingISM->SetCastShadow(false);
	UnitRingISM->SetReceivesDecals(false);
	UnitRingISM->SetVisibility(true);
	UnitRingISM->SetHiddenInGame(false);

	BuildingSelectionBoxISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BuildingSelectionBoxISM"));
	BuildingSelectionBoxISM->SetupAttachment(SceneRoot);
	BuildingSelectionBoxISM->SetMobility(EComponentMobility::Movable);
	BuildingSelectionBoxISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BuildingSelectionBoxISM->SetGenerateOverlapEvents(false);
	BuildingSelectionBoxISM->SetCastShadow(false);
	BuildingSelectionBoxISM->SetReceivesDecals(false);
	BuildingSelectionBoxISM->SetVisibility(true);
	BuildingSelectionBoxISM->SetHiddenInGame(false);

	HPBarISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("HPBarISM"));
	HPBarISM->SetupAttachment(SceneRoot);
	HPBarISM->SetMobility(EComponentMobility::Movable);
	HPBarISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HPBarISM->SetGenerateOverlapEvents(false);
	HPBarISM->SetCastShadow(false);
	HPBarISM->SetReceivesDecals(false);
	HPBarISM->SetVisibility(true);
	HPBarISM->SetHiddenInGame(false);
	HPBarISM->NumCustomDataFloats = HPBAR_NUM_CUSTOMDATA_FLOATS;
}

void ATWSelectionVisualActor::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerSlotColorMap.Num() == 0)
	{
		PlayerSlotColorMap.Add(0, FLinearColor(0.10f, 0.45f, 1.00f, 1.00f));
		PlayerSlotColorMap.Add(1, FLinearColor(1.00f, 0.15f, 0.15f, 1.00f));
		PlayerSlotColorMap.Add(2, FLinearColor(0.15f, 0.90f, 0.20f, 1.00f));
		PlayerSlotColorMap.Add(3, FLinearColor(1.00f, 0.85f, 0.10f, 1.00f));
	}

	ClearVisuals();
}

void ATWSelectionVisualActor::ConfigureRenderAssets(
	UStaticMesh* InUnitRingMesh,
	UMaterialInterface* InUnitRingMaterial,
	float InUnitRingMeshBaseDiameter,
	float InUnitRingZOffset,
	bool bInRotateUnitRingToGroundPlane,
	const FRotator& InUnitRingRotationOffset,
	float InUnitRingScaleMultiplier,
	UStaticMesh* InBuildingSelectionBoxMesh,
	UMaterialInterface* InBuildingSelectionBoxMaterial,
	float InBuildingSelectionBoxThickness,
	UStaticMesh* InHPBarMesh,
	UMaterialInterface* InHPBarMaterial,
	const FVector& InHPBarWorldScale
)
{
	UnitRingMesh = InUnitRingMesh;
	UnitRingMaterial = InUnitRingMaterial;
	UnitRingMeshBaseDiameter = InUnitRingMeshBaseDiameter;
	UnitRingZOffset = InUnitRingZOffset;
	bRotateUnitRingToGroundPlane = bInRotateUnitRingToGroundPlane;
	UnitRingRotationOffset = InUnitRingRotationOffset;
	UnitRingScaleMultiplier = InUnitRingScaleMultiplier;

	BuildingSelectionBoxMesh = InBuildingSelectionBoxMesh;
	BuildingSelectionBoxMaterial = InBuildingSelectionBoxMaterial;
	BuildingSelectionBoxThickness = InBuildingSelectionBoxThickness;

	HPBarMesh = InHPBarMesh;
	HPBarMaterial = InHPBarMaterial;
	HPBarWorldScale = InHPBarWorldScale;

	if (UnitRingISM)
	{
		UnitRingISM->SetStaticMesh(UnitRingMesh);

		if (UnitRingMaterial)
		{
			UnitRingISM->SetMaterial(0, UnitRingMaterial);
			UnitRingMID = UMaterialInstanceDynamic::Create(UnitRingMaterial, this);
			if (UnitRingMID)
			{
				UnitRingISM->SetMaterial(0, UnitRingMID);
			}
		}
	}

	if (BuildingSelectionBoxISM)
	{
		BuildingSelectionBoxISM->SetStaticMesh(BuildingSelectionBoxMesh);

		if (BuildingSelectionBoxMaterial)
		{
			BuildingSelectionBoxISM->SetMaterial(0, BuildingSelectionBoxMaterial);
			BuildingSelectionBoxMID = UMaterialInstanceDynamic::Create(BuildingSelectionBoxMaterial, this);
			if (BuildingSelectionBoxMID)
			{
				BuildingSelectionBoxISM->SetMaterial(0, BuildingSelectionBoxMID);
			}
		}
	}

	if (HPBarISM)
	{
		HPBarISM->SetStaticMesh(HPBarMesh);
		HPBarISM->NumCustomDataFloats = HPBAR_NUM_CUSTOMDATA_FLOATS;

		if (HPBarMaterial)
		{
			HPBarISM->SetMaterial(0, HPBarMaterial);
		}
	}
}

void ATWSelectionVisualActor::SetVisualData(
	const FTWSelectedVisualData& InPrimarySelectedVisualData,
	const TArray<FTWUnitRingVisualData>& InSelectedUnitRingVisuals,
	const TArray<FTWBuildingSelectionVisualData>& InSelectedBuildingVisuals,
	const TArray<FTWHPBarVisualData>& InSelectedHPBarVisuals
)
{
	PrimarySelectedVisualData = InPrimarySelectedVisualData;
	SelectedUnitRingVisuals = InSelectedUnitRingVisuals;
	SelectedBuildingVisuals = InSelectedBuildingVisuals;
	SelectedHPBarVisuals = InSelectedHPBarVisuals;
}

void ATWSelectionVisualActor::ClearVisuals()
{
	PrimarySelectedVisualData.Reset();
	SelectedUnitRingVisuals.Empty();
	SelectedBuildingVisuals.Empty();
	SelectedHPBarVisuals.Empty();

	if (UnitRingISM)
	{
		UnitRingISM->ClearInstances();
		UnitRingISM->MarkRenderStateDirty();
	}

	if (BuildingSelectionBoxISM)
	{
		BuildingSelectionBoxISM->ClearInstances();
		BuildingSelectionBoxISM->MarkRenderStateDirty();
	}

	if (HPBarISM)
	{
		HPBarISM->ClearInstances();
		HPBarISM->MarkRenderStateDirty();
	}
}

void ATWSelectionVisualActor::SyncVisuals()
{
	SyncUnitRingISM();
	SyncBuildingSelectionBoxISM();
	SyncHPBarISM();
}

int32 ATWSelectionVisualActor::ResolveLocalPlayerSlot() const
{
	const AActor* OwnerActor = GetOwner();
	if (const ATWPlayerController* OwnerTWPC = Cast<ATWPlayerController>(OwnerActor))
	{
		if (const ATWPlayerState* TWPS = OwnerTWPC->GetPlayerState<ATWPlayerState>())
		{
			return TWPS->PlayerSlot;
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* PC = World->GetFirstPlayerController())
		{
			if (const ATWPlayerState* TWPS = PC->GetPlayerState<ATWPlayerState>())
			{
				return TWPS->PlayerSlot;
			}
		}
	}

	return INDEX_NONE;
}

FLinearColor ATWSelectionVisualActor::ResolveSlotBaseColor(int32 InOwnerPlayerSlot) const
{
	if (const FLinearColor* FoundColor = PlayerSlotColorMap.Find(InOwnerPlayerSlot))
	{
		return *FoundColor;
	}

	return DefaultSlotColor;
}

FLinearColor ATWSelectionVisualActor::ApplyRelationTint(const FLinearColor& InBaseColor, int32 InOwnerPlayerSlot) const
{
	if (InOwnerPlayerSlot == INDEX_NONE)
	{
		return FLinearColor::LerpUsingHSV(InBaseColor, NeutralColor, NeutralDesaturationAlpha);
	}

	const int32 LocalPlayerSlot = ResolveLocalPlayerSlot();
	if (LocalPlayerSlot == INDEX_NONE)
	{
		return InBaseColor;
	}

	if (LocalPlayerSlot == InOwnerPlayerSlot)
	{
		return InBaseColor;
	}

	FLinearColor EnemyTinted = InBaseColor * EnemyBrightnessMultiplier;
	EnemyTinted.R = FMath::Clamp(EnemyTinted.R, 0.f, 1.f);
	EnemyTinted.G = FMath::Clamp(EnemyTinted.G, 0.f, 1.f);
	EnemyTinted.B = FMath::Clamp(EnemyTinted.B, 0.f, 1.f);
	EnemyTinted.A = 1.f;
	return EnemyTinted;
}

FLinearColor ATWSelectionVisualActor::ResolveFinalOwnerColor(int32 InOwnerPlayerSlot) const
{
	const FLinearColor BaseColor = ResolveSlotBaseColor(InOwnerPlayerSlot);
	return ApplyRelationTint(BaseColor, InOwnerPlayerSlot);
}

void ATWSelectionVisualActor::SyncUnitRingISM()
{
	if (!UnitRingISM || !UnitRingMesh)
	{
		return;
	}

	UnitRingISM->ClearInstances();

	if (SelectedUnitRingVisuals.Num() <= 0)
	{
		UnitRingISM->MarkRenderStateDirty();
		return;
	}

	const float SafeBaseDiameter = FMath::Max(1.f, UnitRingMeshBaseDiameter);
	constexpr float ExtraVisualScale = 1.10f;
	const float SafeZOffset = FMath::Max(UnitRingZOffset, 12.f);

	for (const FTWUnitRingVisualData& RingData : SelectedUnitRingVisuals)
	{
		const float DebugRadius = FMath::Max(1.f, RingData.RingRadius);
		const float VisualRadius = DebugRadius * UnitRingScaleMultiplier * ExtraVisualScale;
		const float VisualDiameter = VisualRadius * 2.f;
		const float ScaleXY = VisualDiameter / SafeBaseDiameter;

		const FVector InstanceLocation(
			RingData.RingWorldLocation.X,
			RingData.RingWorldLocation.Y,
			RingData.RingWorldLocation.Z + SafeZOffset
		);

		const FRotator InstanceRotation =
			bRotateUnitRingToGroundPlane
			? UnitRingRotationOffset
			: FRotator::ZeroRotator;

		const FVector InstanceScale(
			FMath::Max(0.01f, ScaleXY),
			FMath::Max(0.01f, ScaleXY),
			1.0f
		);

		UnitRingISM->AddInstance(FTransform(InstanceRotation, InstanceLocation, InstanceScale), true);
	}

	UnitRingISM->SetHiddenInGame(false);
	UnitRingISM->SetVisibility(true, true);
	UnitRingISM->MarkRenderStateDirty();
}

void ATWSelectionVisualActor::SyncBuildingSelectionBoxISM()
{
	if (!BuildingSelectionBoxISM || !BuildingSelectionBoxMesh)
	{
		return;
	}

	BuildingSelectionBoxISM->ClearInstances();

	if (SelectedBuildingVisuals.Num() <= 0)
	{
		BuildingSelectionBoxISM->MarkRenderStateDirty();
		return;
	}

	for (const FTWBuildingSelectionVisualData& BuildingData : SelectedBuildingVisuals)
	{
		const FVector BoxCenter =
			BuildingData.SelectionWorldLocation +
			FVector(0.f, 0.f, BuildingData.BuildingZOffset);

		const FVector BoxScale(
			FMath::Max(0.01f, (BuildingData.BuildingHalfExtentXY.X * 2.f) / 100.f),
			FMath::Max(0.01f, (BuildingData.BuildingHalfExtentXY.Y * 2.f) / 100.f),
			FMath::Max(0.01f, BuildingSelectionBoxThickness / 100.f)
		);

		BuildingSelectionBoxISM->AddInstance(
			FTransform(FRotator::ZeroRotator, BoxCenter, BoxScale),
			true
		);
	}

	BuildingSelectionBoxISM->SetHiddenInGame(false);
	BuildingSelectionBoxISM->SetVisibility(true, true);
	BuildingSelectionBoxISM->MarkRenderStateDirty();
}

void ATWSelectionVisualActor::SyncHPBarISM()
{
	if (!HPBarISM || !HPBarMesh)
	{
		return;
	}

	HPBarISM->ClearInstances();
	HPBarISM->NumCustomDataFloats = HPBAR_NUM_CUSTOMDATA_FLOATS;

	if (SelectedHPBarVisuals.Num() <= 0)
	{
		HPBarISM->MarkRenderStateDirty();
		return;
	}

	FRotator FacingRotation = FRotator::ZeroRotator;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager)
			{
				const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
				if (SelectedHPBarVisuals.Num() > 0)
				{
					FacingRotation = UKismetMathLibrary::FindLookAtRotation(
						SelectedHPBarVisuals[0].WorldLocation,
						CameraLocation
					);
					FacingRotation.Pitch = 0.f;
					FacingRotation.Roll = 0.f;
				}
			}
		}
	}

	for (const FTWHPBarVisualData& HPBarData : SelectedHPBarVisuals)
	{
		if (!HPBarData.bValid)
		{
			continue;
		}

		const FTransform HPBarTransform(
			FacingRotation,
			HPBarData.WorldLocation,
			HPBarData.WorldScale
		);

		const int32 InstanceIndex = HPBarISM->AddInstance(HPBarTransform, true);
		if (InstanceIndex == INDEX_NONE)
		{
			continue;
		}

		const FLinearColor FinalColor = ResolveFinalOwnerColor(HPBarData.OwnerPlayerSlot);

		HPBarISM->SetCustomDataValue(InstanceIndex, HPBAR_CUSTOMDATA_HEALTH_PERCENT, FMath::Clamp(HPBarData.HealthPercent, 0.f, 1.f), false);
		HPBarISM->SetCustomDataValue(InstanceIndex, HPBAR_CUSTOMDATA_COLOR_R, FinalColor.R, false);
		HPBarISM->SetCustomDataValue(InstanceIndex, HPBAR_CUSTOMDATA_COLOR_G, FinalColor.G, false);
		HPBarISM->SetCustomDataValue(InstanceIndex, HPBAR_CUSTOMDATA_COLOR_B, FinalColor.B, false);
		HPBarISM->SetCustomDataValue(InstanceIndex, HPBAR_CUSTOMDATA_IS_BUILDING, HPBarData.bIsBuilding ? 1.f : 0.f, false);
	}

	HPBarISM->SetHiddenInGame(false);
	HPBarISM->SetVisibility(true, true);
	HPBarISM->MarkRenderStateDirty();
}