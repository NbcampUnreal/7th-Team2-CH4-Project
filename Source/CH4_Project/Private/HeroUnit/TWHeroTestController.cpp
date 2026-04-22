#include "HeroUnit/TWHeroTestController.h"
#include "HeroUnit/TWHeroUnitProjectile.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Log/TWLogCategory.h"


ATWHeroTestController::ATWHeroTestController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void ATWHeroTestController::BeginPlay()
{
    Super::BeginPlay();
    
    if (!IsLocalController()) return;
    
    ControlledHero = Cast<ATWHeroUnitBase>(UGameplayStatics::GetActorOfClass(GetWorld(), ATWHeroUnitBase::StaticClass()));

    if (ControlledHero)
    {
        UE_LOG(LogTWHero, Warning, TEXT("HeroController 캐스트 성공"));
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }
    else
    {
        UE_LOG(LogTWHero, Error, TEXT("HeroController 캐스트 실패"));
    }
}

void ATWHeroTestController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (bIsTargetingMode && ControlledHero)
    {
        ControlledHero->UpdateIndicator(GetMouseWorldLocation());
    }
}

void ATWHeroTestController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        EIC->BindAction(IA_UseSkill, ETriggerEvent::Started, this, &ATWHeroTestController::OnSkillAction);
        EIC->BindAction(IA_Confirm, ETriggerEvent::Started, this, &ATWHeroTestController::OnConfirmAction);
        EIC->BindAction(IA_Cancel, ETriggerEvent::Started, this, &ATWHeroTestController::OnCancelAction);
    }
}

void ATWHeroTestController::OnSkillAction()
{
    if (!ControlledHero)
    {
        return;
    }
    
    if (!ControlledHero->GetSkillReady())
    {
        UE_LOG(LogTWHero, Warning, TEXT("쿨다운 중입니다"));
        return;
    }

    if (ControlledHero->IsIndicatorRequired())
    {
        UE_LOG(LogTWHero, Warning, TEXT("인디케이터 필요 스킬 사용"));
        bIsTargetingMode = !bIsTargetingMode;
        ControlledHero->SetIndicatorVisible(bIsTargetingMode);
        
        if (ControlledHero->IsRangeRequired())
        {
            UE_LOG(LogTWHero, Warning, TEXT("범위 표시 필요 스킬 사용"));
            ControlledHero->SetRangeVisible(bIsTargetingMode);
        }
    }
    else
    {
        UE_LOG(LogTWHero, Warning, TEXT("즉시 시전 스킬 사용"));
        ControlledHero->CurrentTargetLocation = GetMouseWorldLocation();
        ControlledHero->UseSkill();
        bIsTargetingMode = false;
    }
}

void ATWHeroTestController::OnConfirmAction()
{
    if (bIsTargetingMode && ControlledHero)
    {
        UE_LOG(LogTWHero, Warning, TEXT("스킬 시전"));
        ControlledHero->CurrentTargetLocation = GetMouseWorldLocation();
        ControlledHero->UseSkill();
        bIsTargetingMode = false;
    }
}

void ATWHeroTestController::OnCancelAction()
{
    if (bIsTargetingMode && ControlledHero)
    {
        UE_LOG(LogTWHero, Warning, TEXT("스킬 사용 취소"));
        bIsTargetingMode = false;
        ControlledHero->SetIndicatorVisible(bIsTargetingMode);
    }
}

FVector ATWHeroTestController::GetMouseWorldLocation() const
{
    FHitResult Hit;
    // 바닥 감지를 위해 ECC_Visibility 사용 (필요시 전용 채널로 변경)
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);
    return Hit.bBlockingHit ? Hit.ImpactPoint : FVector::ZeroVector;
}
