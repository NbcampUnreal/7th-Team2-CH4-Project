#include "FOW/TWFogManager.h"

#include "Component/TWTeamComponent.h"
#include "Core/TWPlayerState.h"
#include "FOW/TWVisionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TextureResource.h" // RenderTarget 읽기를 위해 필요

ATWFogManager::ATWFogManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ATWFogManager::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();
    if (!World) return;
    
    if (CurrentFogRT && ExploredFogRT)
    {
        CurrentFogRT->UpdateResource();
        ExploredFogRT->UpdateResource();
        UKismetRenderingLibrary::ClearRenderTarget2D(World, CurrentFogRT, FLinearColor::Black);
        UKismetRenderingLibrary::ClearRenderTarget2D(World, ExploredFogRT, FLinearColor::Black);
    }
    
    if (FogPostProcessMaterial)
    {
        FogPostProcessMID = UMaterialInstanceDynamic::Create(FogPostProcessMaterial, this);
        if (FogPostProcessMID)
        {
            FogPostProcessMID->SetTextureParameterValue(TEXT("CurrentRT"), CurrentFogRT);
            FogPostProcessMID->SetTextureParameterValue(TEXT("ExploredRT"), ExploredFogRT);

            TArray<AActor*> FoundVolumes;
            UGameplayStatics::GetAllActorsOfClass(World, APostProcessVolume::StaticClass(), FoundVolumes);
            for (AActor* VolActor : FoundVolumes)
            {
                if (APostProcessVolume* PPV = Cast<APostProcessVolume>(VolActor))
                {
                    PPV->AddOrUpdateBlendable(FogPostProcessMID, 1.0f);
                    PPV->bUnbound = true;
                }
            }
        }
    }

    if (DrawMaterial) DrawMID = UMaterialInstanceDynamic::Create(DrawMaterial, this);
    if (CombineMaterial) CombineMID = UMaterialInstanceDynamic::Create(CombineMaterial, this);
    
    if (FogMPC)
    {
        UKismetMaterialLibrary::SetVectorParameterValue(World, FogMPC, TEXT("MapSize"), FLinearColor(MapSize.X, MapSize.Y, 0, 0));
    }
}

void ATWFogManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    AccumulatedTime += DeltaTime;
    if (AccumulatedTime >= UpdateInterval)
    {
        UpdateFog();
        AccumulatedTime = 0.f;
    }
}

void ATWFogManager::RegisterVision(UTWVisionComponent* Comp)
{
    if (Comp)
    {
        RegisteredVisionComponents.AddUnique(Comp);
    }
}

void ATWFogManager::UnregisterVision(UTWVisionComponent* Comp)
{
    RegisteredVisionComponents.Remove(Comp);
}

FVector2D ATWFogManager::WorldToUV(FVector WorldPos)
{
    float U = ((WorldPos.X - MapOrigin.X) / MapSize.X) + 0.5f;
    float V = ((WorldPos.Y - MapOrigin.Y) / MapSize.Y) + 0.5f;
    return FVector2D(U, V);
}

void ATWFogManager::UpdateFog()
{
    if (!CurrentFogRT || !ExploredFogRT || !DrawMID || !CombineMID) return;
    
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    
    ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
    if (!PS) return;
    
    int32 MyLocalTeamSlot = PS->PlayerSlot;
    if (MyLocalTeamSlot < 0) return;
    
    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CurrentFogRT, FLinearColor::Black);
    
    for (int32 i = RegisteredVisionComponents.Num() - 1; i >= 0; --i)
    {
        UTWVisionComponent* VC = RegisteredVisionComponents[i].Get();
        if (!IsValid(VC) || !IsValid(VC->GetOwner()))
        {
            RegisteredVisionComponents.RemoveAt(i);
            continue;
        }
        
        UTWTeamComponent* TeamComp = VC->GetOwner()->FindComponentByClass<UTWTeamComponent>();
        if (TeamComp && TeamComp->TeamID == MyLocalTeamSlot)
        {
            FVector2D UV = WorldToUV(VC->GetVisionLocation());
            DrawMID->SetVectorParameterValue(TEXT("VisionPos"), FLinearColor(UV.X, UV.Y, 0, 0));
            DrawMID->SetScalarParameterValue(TEXT("Radius"), VC->VisionRadius / MapSize.X);
            
            UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), CurrentFogRT, DrawMID);
        }
    }
    
    CombineMID->SetTextureParameterValue(TEXT("CurrentTex"), CurrentFogRT);
    CombineMID->SetTextureParameterValue(TEXT("PrevTex"), ExploredFogRT);
    UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), ExploredFogRT, CombineMID);
    
    UpdateEnemyVisibility();
}


void ATWFogManager::UpdateEnemyVisibility()
{
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->IsLocalController()) return;

    ATWPlayerState* PS = PC->GetPlayerState<ATWPlayerState>();
    if (!PS || !CurrentFogRT) return;
    
    FTextureResource* TextureResource = CurrentFogRT->GetResource();
    if (!TextureResource)
    {
        CurrentFogRT->UpdateResource();
        return; 
    }

    FTextureRenderTargetResource* RenderTargetResource = static_cast<FTextureRenderTargetResource*>(TextureResource);
    TArray<FColor> RawPixels;
    if (!RenderTargetResource->ReadPixels(RawPixels))
    {
        return;
    }
    
    TArray<AActor*> TaggedUnits;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Unit"), TaggedUnits);

    int32 LocalPlayerTeamID = PS->PlayerSlot;
    int32 RTSizeX = CurrentFogRT->SizeX;
    int32 RTSizeY = CurrentFogRT->SizeY;

    for (AActor* UnitActor : TaggedUnits)
    {
        if (!IsValid(UnitActor)) continue;

        UTWTeamComponent* TeamComp = UnitActor->FindComponentByClass<UTWTeamComponent>();
        if (TeamComp && TeamComp->TeamID == LocalPlayerTeamID)
        {
            if (UnitActor->IsHidden()) UnitActor->SetActorHiddenInGame(false);
            continue;
        }
        
        FVector2D UnitUV = WorldToUV(UnitActor->GetActorLocation());
        if (UnitUV.X < 0.f || UnitUV.X > 1.f || UnitUV.Y < 0.f || UnitUV.Y > 1.f)
        {
            UnitActor->SetActorHiddenInGame(true);
            continue;
        }

        int32 PixelX = FMath::Clamp(static_cast<int32>(UnitUV.X * RTSizeX), 0, RTSizeX - 1);
        int32 PixelY = FMath::Clamp(static_cast<int32>(UnitUV.Y * RTSizeY), 0, RTSizeY - 1);
        int32 PixelIndex = (PixelY * RTSizeX) + PixelX;

        if (RawPixels.IsValidIndex(PixelIndex))
        {
            uint8 Brightness = RawPixels[PixelIndex].R;
            bool bIsVisibleNow = Brightness > 25;
            
            if (UnitActor->IsHidden() == bIsVisibleNow)
            {
                UnitActor->SetActorHiddenInGame(!bIsVisibleNow);
            }
        }
    }
}
