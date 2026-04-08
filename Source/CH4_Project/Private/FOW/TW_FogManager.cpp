#include "FOW/TW_FogManager.h"

#include "Component/TW_TeamComponent.h"
#include "FOW/Test_MyPlayerState.h"
#include "FOW/TW_VisionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"

ATW_FogManager::ATW_FogManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ATW_FogManager::BeginPlay()
{
    Super::BeginPlay();
    UWorld* World = GetWorld();
    if (!World) return;
    
    CurrentFogRT = UKismetRenderingLibrary::CreateRenderTarget2D(World, 1024, 1024, RTF_R8);
    ExploredFogRT = UKismetRenderingLibrary::CreateRenderTarget2D(World, 1024, 1024, RTF_R8);
    
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

void ATW_FogManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    AccumulatedTime += DeltaTime;
    if (AccumulatedTime >= UpdateInterval)
    {
        UpdateFog();
        AccumulatedTime = 0.f;
    }
}

void ATW_FogManager::RegisterVision(UTW_VisionComponent* Comp) { if (Comp) RegisteredVisionComponents.AddUnique(Comp); }
void ATW_FogManager::UnregisterVision(UTW_VisionComponent* Comp) { RegisteredVisionComponents.Remove(Comp); }

FVector2D ATW_FogManager::WorldToUV(FVector WorldPos)
{
    float U = ((WorldPos.X - MapOrigin.X) / MapSize.X) + 0.5f;
    float V = ((WorldPos.Y - MapOrigin.Y) / MapSize.Y) + 0.5f;
    return FVector2D(U, V);
}

void ATW_FogManager::UpdateFog()
{
    if (!CurrentFogRT || !ExploredFogRT || !DrawMID || !CombineMID) return;
    
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (!PlayerController) return;
    
    // 테스트용 플레이어 스테이트 넣어서 확인 > 플레이어 스테이트 추가해서 변경해줘야함
    ATest_MyPlayerState* PlayerState = PlayerController->GetPlayerState<ATest_MyPlayerState>();
    if (!PlayerState) return;
    
    int32 MyLocalTeamID = PlayerState->TeamID;
    if (MyLocalTeamID <= 0) return;
    
    UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(), CurrentFogRT, FLinearColor::Black);
    
    for (int32 i = RegisteredVisionComponents.Num() - 1; i >= 0; --i)
    {
        UTW_VisionComponent* VC = RegisteredVisionComponents[i].Get();
        if (!IsValid(VC) || !IsValid(VC->GetOwner()))
        {
            RegisteredVisionComponents.RemoveAt(i);
            continue;
        }
        
        UTW_TeamComponent* TeamComp = VC->GetOwner()->FindComponentByClass<UTW_TeamComponent>();
        if (TeamComp && TeamComp->TeamID == MyLocalTeamID)
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
}
