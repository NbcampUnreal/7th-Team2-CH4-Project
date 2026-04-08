#include "CapturePoint/TW_CapturePoint.h"

#include "Component/TW_TeamComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "FOW/TW_VisionComponent.h"
#include "Net/UnrealNetwork.h"

ATW_CapturePoint::ATW_CapturePoint()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    CaptureArea = CreateDefaultSubobject<UBoxComponent>(TEXT("CaptureArea"));
    RootComponent = CaptureArea;
    CaptureArea->SetCollisionProfileName(TEXT("Trigger"));

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    MeshComp->SetupAttachment(RootComponent);
    
    MyTeamComp = CreateDefaultSubobject<UTW_TeamComponent>(TEXT("TeamComponent"));
    
    MyVisionComp = CreateDefaultSubobject<UTW_VisionComponent>(TEXT("VisionComponent"));
    if (MyVisionComp)
    {
        MyVisionComp->VisionRadius = VisionRad;
    }
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

void ATW_CapturePoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority()) return;

    if (OtherActor && OtherActor->FindComponentByClass<UTW_TeamComponent>())
    {
        OverlappingActors.Add(OtherActor);
        StartCaptureTimer();
    }
}

void ATW_CapturePoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority()) return;
    
    OverlappingActors.Remove(OtherActor);
}

void ATW_CapturePoint::StartCaptureTimer()
{
    if (!HasAuthority()) return;
    
    if (!GetWorld()->GetTimerManager().IsTimerActive(CaptureTimerHandle))
    {
        GetWorld()->GetTimerManager().SetTimer(CaptureTimerHandle, this, &ATW_CapturePoint::CheckCaptureStatus, CheckInterval, true);
    }
}

void ATW_CapturePoint::StopCaptureTimer()
{
    if (!HasAuthority()) return;
    
    if (GetWorld()->GetTimerManager().IsTimerActive(CaptureTimerHandle))
    {
        GetWorld()->GetTimerManager().ClearTimer(CaptureTimerHandle);
        UE_LOG(LogTemp, Log, TEXT("=== [SERVER] Capture Timer Stopped ==="));
    }
}

void ATW_CapturePoint::CheckCaptureStatus()
{
    if (!HasAuthority()) return;

    TArray<AActor*> CurrentlyInArea;
    CaptureArea->GetOverlappingActors(CurrentlyInArea, APawn::StaticClass());
    
    TSet<int32> PresentTeams;
    for (AActor* Actor : CurrentlyInArea)
    {
        if (UTW_TeamComponent* TeamComp = Actor->FindComponentByClass<UTW_TeamComponent>())
        {
            if (TeamComp->TeamID > 0) 
            {
                PresentTeams.Add(TeamComp->TeamID);
            }
        }
    }

    int32 TeamCount = PresentTeams.Num();
    int32 ActorCount = CurrentlyInArea.Num(); // 팀 상관없이 물리적으로 있는 인원수
    
    if (TeamCount == 1)
    {
        int32 CurrentUnitTeam = *PresentTeams.CreateConstIterator();

        if (CurrentUnitTeam == MyTeamComp->TeamID)
        {
            CurrentGauge = MaxGauge;
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
                UE_LOG(LogTemp, Warning, TEXT("=== [SERVER] Capture Complete! Team %d ==="), CurrentUnitTeam);
            }
        }
    }
    else if (TeamCount >= 2)
    {
        //격돌중
    }
    else
    {
        if (CurrentGauge > 0.0f)
        {
            CurrentGauge = FMath::Max(CurrentGauge - (DecaySpeed * CheckInterval), 0.0f);
        }
        else if (ActorCount == 0)
        {
            StopCaptureTimer();
            CapturingTeamID = 0;
            return;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("=== [SERVER] Progress: %.1f%% (Present Teams: %d) ==="), CurrentGauge, TeamCount);
}

void ATW_CapturePoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ATW_CapturePoint, CurrentGauge);
}
