#include "UI/World/TWFloatingTextActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ATWFloatingTextActor::ATWFloatingTextActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetReplicateMovement(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
	TextRender->SetupAttachment(Root);
	TextRender->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	TextRender->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	TextRender->SetWorldSize(DefaultWorldSize);
	TextRender->SetTextRenderColor(FColor::White);
	TextRender->SetHiddenInGame(false);
	TextRender->SetCastShadow(false);
	TextRender->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TextRender->SetUsingAbsoluteRotation(true);

	// TextRender가 프로젝트/카메라 기준으로 뒤집혀 보이는 경우가 많아서 기본 보정
	TextRender->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	TextRender->SetXScale(DefaultXScale);
	TextRender->SetYScale(DefaultYScale);
}

void ATWFloatingTextActor::BeginPlay()
{
	Super::BeginPlay();
	UpdateFacingToLocalCamera();
}

void ATWFloatingTextActor::Destroyed()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(UpdateTimerHandle);
		GetWorldTimerManager().ClearTimer(DestroyTimerHandle);
	}

	Super::Destroyed();
}

void ATWFloatingTextActor::InitializeText(
	const FString& InMessage,
	const FColor& InColor,
	float InLifetime,
	float InRiseSpeed
)
{
	Lifetime = FMath::Max(0.1f, InLifetime);
	RiseSpeed = InRiseSpeed;
	ElapsedTime = 0.0f;

	if (TextRender)
	{
		TextRender->SetText(FText::FromString(InMessage));
		TextRender->SetTextRenderColor(InColor);
		TextRender->SetWorldSize(DefaultWorldSize);
		TextRender->SetXScale(DefaultXScale);
		TextRender->SetYScale(DefaultYScale);
	}

	UpdateFacingToLocalCamera();

	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			UpdateTimerHandle,
			this,
			&ATWFloatingTextActor::UpdateFloatingText,
			UpdateInterval,
			true
		);

		GetWorldTimerManager().SetTimer(
			DestroyTimerHandle,
			this,
			&ATWFloatingTextActor::FinishFloatingText,
			Lifetime,
			false
		);
	}
}

void ATWFloatingTextActor::UpdateFloatingText()
{
	const float DeltaZ = RiseSpeed * UpdateInterval;
	AddActorWorldOffset(FVector(0.0f, 0.0f, DeltaZ));

	ElapsedTime += UpdateInterval;

	const float AlphaRatio = 1.0f - FMath::Clamp(ElapsedTime / Lifetime, 0.0f, 1.0f);

	if (TextRender)
	{
		FColor CurrentColor = TextRender->TextRenderColor;
		CurrentColor.A = static_cast<uint8>(255.0f * AlphaRatio);
		TextRender->SetTextRenderColor(CurrentColor);
	}

	UpdateFacingToLocalCamera();
}

void ATWFloatingTextActor::FinishFloatingText()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(UpdateTimerHandle);
		GetWorldTimerManager().ClearTimer(DestroyTimerHandle);
	}

	Destroy();
}

void ATWFloatingTextActor::UpdateFacingToLocalCamera()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* LocalPC = World->GetFirstPlayerController();
	if (!LocalPC || !LocalPC->IsLocalController() || !LocalPC->PlayerCameraManager)
	{
		return;
	}

	const FVector CameraLocation = LocalPC->PlayerCameraManager->GetCameraLocation();
	const FVector TextLocation = GetActorLocation();

	FRotator LookAtRotation = (CameraLocation - TextLocation).Rotation();
	
	LookAtRotation.Pitch = 0.0f;
	LookAtRotation.Roll = 0.0f;

	SetActorRotation(LookAtRotation);

	const float Distance = FVector::Distance(CameraLocation, TextLocation);
	const float DistanceScale = FMath::Clamp(
		Distance / DistanceScaleDivisor,
		MinDistanceScale,
		MaxDistanceScale
	);

	SetActorScale3D(FVector(DistanceScale));
}