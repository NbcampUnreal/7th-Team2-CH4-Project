#include "CapturePoint/TW_CapturePoint.h"

#include "Component/TWTeamComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/TWResourceBuildingDataAsset.h"
#include "TimerManager.h"
#include "Component/TWTeamColorComponent.h"
#include "Components/WidgetComponent.h"
#include "Core/TWPlayerState.h"
#include "FOW/TWVisionComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "UI/Widgets/TWCaptureGaugeWidget.h"

ATW_CapturePoint::ATW_CapturePoint()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    
    MyTeamComp = CreateDefaultSubobject<UTWTeamComponent>(TEXT("TeamComp"));
    MyVisionComp = CreateDefaultSubobject<UTWVisionComponent>(TEXT("VisionComp"));

    CaptureArea = CreateDefaultSubobject<UBoxComponent>(TEXT("CaptureArea"));
    RootComponent = CaptureArea;
    CaptureArea->SetCollisionProfileName(TEXT("Trigger"));

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComp->SetupAttachment(RootComponent);
   
    TeamColorComponent = CreateDefaultSubobject<UTWTeamColorComponent>(TEXT("TeamColorComponent"));
    
    if (MyVisionComp)
    {
        MyVisionComp->VisionRadius = VisionRad;
    }
    
    GaugeWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("GaugeWidget"));
    GaugeWidgetComp->SetupAttachment(RootComponent);
    GaugeWidgetComp->SetWidgetSpace(EWidgetSpace::World);
    GaugeWidgetComp->SetDrawSize(FVector2D(150.0f, 30.0f));
    GaugeWidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
}

void ATW_CapturePoint::BeginPlay()
{
    Super::BeginPlay();
   
    if (HasAuthority() && CaptureArea)
    {
        CaptureArea->OnComponentBeginOverlap.AddDynamic(this, &ATW_CapturePoint::OnOverlapBegin);
        CaptureArea->OnComponentEndOverlap.AddDynamic(this, &ATW_CapturePoint::OnOverlapEnd);
    }
}

void ATW_CapturePoint::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    
    if (TeamColorComponent && MeshComp)
    {
        TeamColorComponent->SetUpTargetMesh(MeshComp);
    }
}

void ATW_CapturePoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority())
    {
        return;
    }

    if (OtherActor && OtherActor->FindComponentByClass<UTWTeamComponent>())
    {
        OverlappingActors.Add(OtherActor);
        StartCaptureTimer();
    }
}

void ATW_CapturePoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority())
    {
        return;
    }
    
    OverlappingActors.Remove(OtherActor);
}

void ATW_CapturePoint::StartCaptureTimer()
{
    if (!HasAuthority())
    {
        return;
    }
    if (GetWorldTimerManager().IsTimerActive(CaptureTimerHandle))
    {
        return;
    }
    
    GetWorldTimerManager().SetTimer(CaptureTimerHandle, this, &ATW_CapturePoint::CheckCaptureStatus, CheckInterval, true);
}

void ATW_CapturePoint::StopCaptureTimer()
{
    if (!HasAuthority())
    {
        return;
    }
    
    if (GetWorld()->GetTimerManager().IsTimerActive(CaptureTimerHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(CaptureTimerHandle);
    }
}

void ATW_CapturePoint::CheckCaptureStatus()
{
    if (!HasAuthority())
    {
        return;
    }
    
    if (!CaptureArea)
    {
        StopCaptureTimer();
        return;
    }
    
    if (!MyTeamComp) 
    {
        StopCaptureTimer();
        return;
    }

    TArray<AActor*> CurrentlyInArea;
    CaptureArea->GetOverlappingActors(CurrentlyInArea, AActor::StaticClass());
    
    int32 ActorCount = CurrentlyInArea.Num(); 
    
    TSet<int32> PresentTeams;
    for (AActor* Actor : CurrentlyInArea)
    {
        if (Actor == this)
        {
            continue;
        }
        
        UTWTeamComponent* TeamComp = Actor->FindComponentByClass<UTWTeamComponent>();
        if (TeamComp)
        {
            if (TeamComp->GetTeamID() >= 0) 
            {
                PresentTeams.Add(TeamComp->GetTeamID());
            }
        }
    }

    int32 TeamCount = PresentTeams.Num();
    if (TeamCount == 1)
    {
        int32 CurrentUnitTeam = *PresentTeams.CreateConstIterator();
        
        if (CurrentUnitTeam == MyTeamComp->GetTeamID())
        {
            CapturingTeamID = CurrentUnitTeam;
            
        }
        else
        {
            if (CapturingTeamID != CurrentUnitTeam)
            {
                CurrentGauge = 0.0f;
                CapturingTeamID = CurrentUnitTeam;
            }
            CurrentGauge = FMath::Min(CurrentGauge + (CaptureSpeed * CheckInterval), MaxGauge);
            
            if (CurrentGauge >= MaxGauge)
            {
                MyTeamComp->SetTeamID(CurrentUnitTeam);
                
                if (CurrentlyInArea.Num() > 0 && CurrentlyInArea[0])
                {
                    // if (APawn* CapturePawn = Cast<APawn>(CurrentlyInArea[0]))
                    // {
                    //     // 이 유닛을 소유한 플레이어의 State를 가져옴
                    //     if (ATWPlayerState* PS = Cast<ATWPlayerState>(CapturePawn->GetPlayerState()))
                    //     {
                    //         SetOwningPlayer(PS);
                    //     }
                    // }
                    
                    if (AGameStateBase* GameState = GetWorld()->GetGameState())
                    {
                        for (APlayerState* PS : GameState->PlayerArray)
                        {
                            if (ATWPlayerState * TWPS = Cast<ATWPlayerState>(PS))
                            {
                                if (TWPS->PlayerSlot == CurrentUnitTeam)
                                {
                                    SetOwningPlayer(TWPS);
                                    break;   
                                }
                            }
                        }
                    }
                }
                UE_LOG(LogTemp, Warning, TEXT("=== [SERVER] Capture Complete! Team %d ==="), CurrentUnitTeam);
            }
        }
    }
    else if (TeamCount >= 2)
    {
        UE_LOG(LogTemp, Error, TEXT("=== [SERVER] Clashing!!! ==="));
    }
    else
    {
        if (CurrentGauge > 0.0f)
        {
            CurrentGauge = FMath::Max(CurrentGauge - (DecaySpeed * CheckInterval), 0.0f);
        }
        else if (ActorCount == 0 && CurrentGauge <= 0.0f)
        {
            StopCaptureTimer();
            CapturingTeamID = -1;
            return;
        }
    }
    UpdateWidgetUI();
    UE_LOG(LogTemp, Log, TEXT("=== [SERVER] Progress: %.1f%% (Present Teams: %d) ==="), CurrentGauge, TeamCount);
}

void ATW_CapturePoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATW_CapturePoint, CurrentGauge);
    DOREPLIFETIME(ATW_CapturePoint, OwningPlayerState);
    DOREPLIFETIME(ATW_CapturePoint, OwnerPlayerSlot);
    DOREPLIFETIME(ATW_CapturePoint, CapturingTeamID);
}

void ATW_CapturePoint::SetOwningPlayer(ATWPlayerState* NewPlayerState)
{
    if (!HasAuthority())
    {
        return;
    }
    
    OwningPlayerState = nullptr;
    
    if (NewPlayerState)
    {
        OwningPlayerState = NewPlayerState;
        StartMithrilProduction();
        
        SetOwnerPlayerSlot(OwningPlayerState->PlayerSlot);
    }
    
}

void ATW_CapturePoint::StartMithrilProduction()
{
    if (!HasAuthority())
    {
        return;
    }
    
    if (!ResourceDataAsset || !OwningPlayerState)
    {
        return;
    }

    if (ResourceDataAsset->ProducedResourceType == EResourceType::None)
    {
        return;
    }

    if (ResourceDataAsset->ProductionInterval <= 0.0f)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(MithrilProductionTimerHandle);
    
    GetWorldTimerManager().SetTimer(
        MithrilProductionTimerHandle,
        this,
        &ATW_CapturePoint::HandleMithrilResource,
        ResourceDataAsset->ProductionInterval,
        true
    );
}

void ATW_CapturePoint::HandleMithrilResource()
{
    if (!HasAuthority())
    {
        return;
    }
    
    if (!ResourceDataAsset || !OwningPlayerState)
    {
        return;
    }

    OwningPlayerState->AddResource(
        ResourceDataAsset->ProducedResourceType,
        ResourceDataAsset->ProductionAmount
    );
}

void ATW_CapturePoint::SetOwnerPlayerSlot(int32 InSlot)
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

void ATW_CapturePoint::OnRep_OwnerPlayerSlot()
{
    if (TeamColorComponent)
    {
        TeamColorComponent->ApplyTeamColor(OwnerPlayerSlot);
    }
}

#pragma region UI

void ATW_CapturePoint::OnRep_CurrentGauge()
{
    UpdateWidgetUI();
}

void ATW_CapturePoint::OnRep_CapturingTeamID()
{
    UpdateWidgetUI();
}

void ATW_CapturePoint::UpdateWidgetUI()
{
    if (UTWCaptureGaugeWidget* GaugeWidget = Cast<UTWCaptureGaugeWidget>(GaugeWidgetComp->GetUserWidgetObject()))
    {
        GaugeWidget->UpdateGauge(CurrentGauge, MaxGauge, CapturingTeamID);
    }
}

#pragma endregion