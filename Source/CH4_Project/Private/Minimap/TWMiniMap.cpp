#include "Minimap/TWMiniMap.h"

#include "Components/SceneCaptureComponent2D.h"

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
		
		CaptureComp->OrthoWidth = CaptureWidth;
		
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
	
	// UE_LOG(LogTemp, Warning, TEXT("미니맵 캡처 실행중"));
	CaptureComp->CaptureScene();
}
