#include "Minimap/TWMiniMap.h"

#include "Component/TWTeamComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Core/TWPlayerState.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "FOW/TWFogManager.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystems/TWBuildingManagerSubsystem.h"
#include "Subsystems/TWGridSubSystem.h"
#include "Building/TWBaseBuilding.h"
#include "CapturePoint/TW_CapturePoint.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

ATWMiniMap::ATWMiniMap()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>("Root");
	
	CaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>("CaptureComponent");
	CaptureComp->SetupAttachment(RootComponent);
	
	CaptureComp->SetRelativeRotation(FRotator(-90.f,0.f,0.f));
	
	CaptureComp->ProjectionType = ECameraProjectionMode::Orthographic;
}

void ATWMiniMap::BeginPlay()
{
	Super::BeginPlay();

	if (CaptureComp)
	{
		if (MinimapRT)
		{
			CaptureComp->TextureTarget = MinimapRT;
		}

		if (UTWGridSubSystem* GridSub = GetWorld()->GetSubsystem<UTWGridSubSystem>())
		{
			GridOrigin = GridSub->GetGridOrigin();
			GridFullSize = GridSub->GetGridFullSize();

			FVector CenterPos = GridOrigin + FVector(GridFullSize.X * 0.5f, GridFullSize.Y * 0.5f, 5000.f);
			SetActorLocation(CenterPos);

			CaptureWidth = GridFullSize.X;
			CaptureComp->OrthoWidth = CaptureWidth;
		}
		else
		{
			CaptureComp->OrthoWidth = CaptureWidth;
		}
        
		CaptureComp->bCaptureEveryFrame = false;
		CaptureComp->bCaptureOnMovement = false;
	}
	
	CacheWaterActors();
	
	GetWorldTimerManager().SetTimer(
		CaptureTimerHandle,
		this,
		&ATWMiniMap::UpdateCapture,
		0.1,
		true
	);
}

void ATWMiniMap::UpdateCapture()
{
	if (!CaptureComp)
	{
		return;
	}
	
	if (CachedLocalTeamSlot == -1)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			if (ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>())
			{
				CachedLocalTeamSlot = PS->PlayerSlot;
			}
		}
	}
	
	ApplyMinimapWaterMaterial();
	CaptureComp->CaptureScene();
	RestoreWaterMaterials();

	DrawCameraFrustum();
	DrawIconsOnMinimap();
}

void ATWMiniMap::DrawCameraFrustum()
{
	if (!FrustumRT)
	{
		return;
	}
	
	UKismetRenderingLibrary::ClearRenderTarget2D(this, FrustumRT, FLinearColor::Transparent);
	
    UCanvas* Canvas;
    FVector2D Size;
    FDrawToRenderTargetContext Context;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, FrustumRT, Canvas, Size, Context);
	
	UKismetRenderingLibrary::ClearRenderTarget2D(
	this,
	FrustumRT,
	FLinearColor::Transparent
	);
	
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && GEngine && GEngine->GameViewport)
    {
        FVector2D ViewportSize;
        GEngine->GameViewport->GetViewportSize(ViewportSize);
    	
        TArray<FVector2D> ScreenPoints = {
            FVector2D(0, 0),
            FVector2D(ViewportSize.X, 0),
            FVector2D(ViewportSize.X, ViewportSize.Y),
            FVector2D(0, ViewportSize.Y)
        };

        TArray<FVector2D> MinimapPoints;
        FVector MapLocation = GetActorLocation();

        for (const FVector2D& Pos : ScreenPoints)
        {
            FVector WorldLoc, WorldDir;
            if (PC->DeprojectScreenPositionToWorld (Pos.X, Pos.Y, WorldLoc, WorldDir))
            {
                float T = -WorldLoc.Z / WorldDir.Z;
                FVector GroundPos = WorldLoc + (WorldDir * T);
            	
            	float U = (GroundPos.X - GridOrigin.X) / GridFullSize.X;
            	float V = (GroundPos.Y - GridOrigin.Y) / GridFullSize.Y;

                MinimapPoints.Add(FVector2D(U * Size.X, V * Size.Y));
            }
        }
        if (MinimapPoints.Num() == 4)
        {
            for (int32 i = 0; i < 4; ++i)
            {
                Canvas->K2_DrawLine(MinimapPoints[i], MinimapPoints[(i + 1) % 4], FrustumThickness, FrustumColor);
            }
        }
    }
    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

void ATWMiniMap::DrawIconsOnMinimap()
{
	if (!IconRT)
	{
		return;
	}
	
    UKismetRenderingLibrary::ClearRenderTarget2D(this, IconRT, FLinearColor::Transparent);

    UCanvas* Canvas;
    FVector2D Size;
    FDrawToRenderTargetContext Context;
    UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, IconRT, Canvas, Size, Context);

    if (!Canvas)
    {
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
        return;
    }
	
    ATWFogManager* FogManager = nullptr;
    {
        TArray<AActor*> Found;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATWFogManager::StaticClass(), Found);
        if (Found.Num() > 0)
            FogManager = Cast<ATWFogManager>(Found[0]);
    }

    int32 LocalTeamSlot = CachedLocalTeamSlot;
    if (LocalTeamSlot == -1)
    {
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
        return;
    }

    // Unit 아이콘
    TArray<AActor*> AllUnits;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Unit"), AllUnits);

    for (AActor* Actor : AllUnits)
    {
        if (!IsValid(Actor))
        {
	        continue;
        }
    	
        UTWTeamComponent* TeamComp = Actor->FindComponentByClass<UTWTeamComponent>();
        if (!TeamComp)
        {
	        continue;
        }
    	
        int32 TeamID = TeamComp->GetTeamID();
    	
        if (!ShouldShow(TeamID, Actor->GetActorLocation(), LocalTeamSlot, FogManager))
        {
            continue;
        }
    	
        FVector2D MinimapPos = WorldToMinimap(Actor->GetActorLocation(), Size);
        float Half = UnitIconSize * 0.5f;
        FLinearColor Color = (TeamID == LocalTeamSlot) ? MyTeamColor : EnemyTeamColor;

        Canvas->K2_DrawBox(MinimapPos - FVector2D(Half, Half),
                           FVector2D(UnitIconSize, UnitIconSize),
                           IconThickness, Color);
    }
	
	// 건물 아이콘
    if (UTWBuildingManagerSubsystem* BuildingSub = GetWorld()->GetSubsystem<UTWBuildingManagerSubsystem>())
    {
        for (ATWBaseBuilding* Building : BuildingSub->GetAllBuildings())
        {
            if (!IsValid(Building))
            {
	            continue;
            }
        	
            UTWTeamComponent* TeamComp = Building->FindComponentByClass<UTWTeamComponent>();
            if (!TeamComp)
            {
	            continue;
            }
        	
            int32 TeamID = TeamComp->GetTeamID();
        	
            if (!ShouldShow(TeamID, Building->GetActorLocation(), LocalTeamSlot, FogManager)) continue;
        	
            FVector2D MinimapPos = WorldToMinimap(Building->GetActorLocation(), Size);
            float Half = BuildingIconSize * 0.5f;
            FLinearColor Color = (TeamID == LocalTeamSlot) ? MyTeamColor : EnemyTeamColor;

            Canvas->K2_DrawBox(MinimapPos - FVector2D(Half, Half),
                               FVector2D(BuildingIconSize, BuildingIconSize),
                               IconThickness, Color);
        }
    }
	
	// 점령지 아이콘
    TArray<AActor*> CapturePoints;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATW_CapturePoint::StaticClass(), CapturePoints);

    for (AActor* Actor : CapturePoints)
    {
        if (!IsValid(Actor))
        {
	        continue;
        }
    	
        UTWTeamComponent* TeamComp = Actor->FindComponentByClass<UTWTeamComponent>();
        if (!TeamComp)
        {
	        continue;
        }
    	
        int32 TeamID = TeamComp->GetTeamID();
    	
        if (!ShouldShow(TeamID, Actor->GetActorLocation(), LocalTeamSlot, FogManager)) continue;
    	
        FVector2D MinimapPos = WorldToMinimap(Actor->GetActorLocation(), Size);
        float Half = CapturePointIconSize * 0.5f;

        FLinearColor Color;
        if (TeamID == -1) Color = NeutralColor;
        else if (TeamID == LocalTeamSlot) Color = MyTeamColor;
        else Color = EnemyTeamColor;
        
        Canvas->K2_DrawBox(MinimapPos - FVector2D(Half, Half),
                           FVector2D(CapturePointIconSize, CapturePointIconSize),
                           IconThickness, Color);
    }

    UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

FVector2D ATWMiniMap::WorldToMinimap(FVector WorldPos, FVector2D CanvasSize) const
{
	float U = (WorldPos.X - GridOrigin.X) / GridFullSize.X;
	float V = (WorldPos.Y - GridOrigin.Y) / GridFullSize.Y;
	return FVector2D(U * CanvasSize.X, V * CanvasSize.Y);
}

bool ATWMiniMap::ShouldShow(int32 TeamID, FVector WorldPos, int32 LocalTeamSlot, class ATWFogManager* FogManager) const
{
	if (TeamID == LocalTeamSlot)
	{
		return true;
	}
	if (!FogManager)
	{
		return true;
	}
	
	return FogManager->IsLocationVisible(WorldPos);
}

void ATWMiniMap::CacheWaterActors()
{
	WaterMeshComponents.Empty();
	OriginalWaterMaterials.Empty();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AStaticMeshActor::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		const FString LowerName = Actor->GetName().ToLower();

		if (!LowerName.Contains(TEXT("water")))
		{
			continue;
		}

		AStaticMeshActor* WaterActor = Cast<AStaticMeshActor>(Actor);
		if (!WaterActor)
		{
			continue;
		}

		UStaticMeshComponent* MeshComp = WaterActor->GetStaticMeshComponent();
		if (!MeshComp)
		{
			continue;
		}

		WaterMeshComponents.Add(MeshComp);

		TArray<UMaterialInterface*> SavedMaterials;
		const int32 MaterialCount = MeshComp->GetNumMaterials();

		for (int32 Index = 0; Index < MaterialCount; ++Index)
		{
			SavedMaterials.Add(MeshComp->GetMaterial(Index));
		}

		OriginalWaterMaterials.Add(MeshComp, SavedMaterials);
	}
}

void ATWMiniMap::ApplyMinimapWaterMaterial()
{
	if (!MinimapWaterMaterial)
	{
		return;
	}

	for (const TWeakObjectPtr<UStaticMeshComponent>& MeshPtr : WaterMeshComponents)
	{
		UStaticMeshComponent* MeshComp = MeshPtr.Get();
		if (!MeshComp)
		{
			continue;
		}

		const int32 MaterialCount = MeshComp->GetNumMaterials();
		for (int32 Index = 0; Index < MaterialCount; ++Index)
		{
			MeshComp->SetMaterial(Index, MinimapWaterMaterial);
		}
	}
}

void ATWMiniMap::RestoreWaterMaterials()
{
	for (TPair<TWeakObjectPtr<UStaticMeshComponent>, TArray<UMaterialInterface*>>& Pair : OriginalWaterMaterials)
	{
		UStaticMeshComponent* MeshComp = Pair.Key.Get();
		if (!MeshComp)
		{
			continue;
		}

		const TArray<UMaterialInterface*>& SavedMaterials = Pair.Value;
		for (int32 Index = 0; Index < SavedMaterials.Num(); ++Index)
		{
			MeshComp->SetMaterial(Index, SavedMaterials[Index]);
		}
	}
}

FVector ATWMiniMap::GetWorldLocationFromTouch(FVector2D TouchPos, FVector2D WidgetSize)
{
	float U = TouchPos.X / WidgetSize.X;
	float V = TouchPos.Y / WidgetSize.Y;

	float WorldX = GridOrigin.X + (1.0f - V) * GridFullSize.X;
	float WorldY = GridOrigin.Y + U * GridFullSize.Y;

	return FVector(WorldX, WorldY, 0.0f);
}
