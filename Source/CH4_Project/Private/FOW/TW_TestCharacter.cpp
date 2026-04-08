#include "FOW/TW_TestCharacter.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Component/TW_TeamComponent.h"
#include "FOW/Test_MyPlayerState.h"
#include "FOW/TW_VisionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

ATW_TestCharacter::ATW_TestCharacter()
{
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 1200.f;
    CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
    CameraBoom->bInheritYaw = false;

    TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    
    TeamComp = CreateDefaultSubobject<UTW_TeamComponent>(TEXT("TeamComponent"));
    if (TeamComp)
    {
        TeamComp->SetTeamID(0);
    }
    
    VisionComp = CreateDefaultSubobject<UTW_VisionComponent>(TEXT("VisionComponent"));
    if (VisionComp)
    {
        VisionComp->VisionRadius = 800.0f;
    }
}

void ATW_TestCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    if (ATest_MyPlayerState* PS = NewController->GetPlayerState<ATest_MyPlayerState>())
    {
        if (TeamComp)
        {
            TeamComp->SetTeamID(PS->TeamID);
        }
    }
}

void ATW_TestCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    SyncTeamFromPlayerState();
}

void ATW_TestCharacter::SyncTeamFromPlayerState()
{
    if (ATest_MyPlayerState* TWPS = GetPlayerState<ATest_MyPlayerState>())
    {
        if (TWPS->TeamID > 0 && TeamComp) 
        {
            TeamComp->SetTeamID(TWPS->TeamID);
        }
    }
}

void ATW_TestCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void ATW_TestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // 2. Enhanced Input Component로 캐스팅 후 바인딩
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATW_TestCharacter::Move);
    }
}

void ATW_TestCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddMovementInput(FVector::ForwardVector, MovementVector.Y);
        AddMovementInput(FVector::RightVector, MovementVector.X);
    }
}
