#include "Minimap/TWMiniMap.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "Subsystems/TWGridSubSystem.h"

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
	
	CaptureComp->CaptureScene();
	DrawCameraFrustum();
}

void ATWMiniMap::DrawCameraFrustum()
{
	if (!FrustumRT) return;
	
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
            	
                float U = (GroundPos.X - MapLocation.X) / CaptureWidth + 0.5f;
                float V = (GroundPos.Y - MapLocation.Y) / CaptureWidth + 0.5f;

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

FVector ATWMiniMap::GetWorldLocationFromTouch(FVector2D TouchPos, FVector2D WidgetSize)
{
	float U = TouchPos.X / WidgetSize.X;
	float V = TouchPos.Y / WidgetSize.Y;
	
	float RelativeX = (U - 0.5f) * CaptureWidth;
	float RelativeY = (V - 0.5f) * CaptureWidth;
	
	float WorldX = -RelativeY; 
	float WorldY = RelativeX;

	FVector MapLocation = GetActorLocation();
	return FVector(MapLocation.X + WorldX, MapLocation.Y + WorldY, 0.0f);
}
